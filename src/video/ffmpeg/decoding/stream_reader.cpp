#include "stream_reader.hpp"

#include <core/log/log.hpp>

#include <core/exception/assert.hpp>

// StreamInfo
namespace step::video::ff {

StreamReader::StreamInfo::StreamInfo() : m_demuxed_stream(nullptr) { reset_state_before_seek(); }

StreamReader::StreamInfo::~StreamInfo() { set_stream(nullptr); }

DemuxedStream* StreamReader::StreamInfo::get_stream() const { return m_demuxed_stream; }

void StreamReader::StreamInfo::set_stream(DemuxedStream* stream)
{
    if (stream)
    {
        if (m_demuxed_stream)
            STEP_THROW_RUNTIME("Previous stream was not released");

        m_demuxed_stream = stream;
    }
    else
    {
        STEP_ASSERT(!m_demuxed_stream, "Invalid set_stream logic");
    }
}

bool StreamReader::StreamInfo::is_stream_enabled() const { return m_demuxed_stream; }

void StreamReader::StreamInfo::unlink_from_reader()
{
    DemuxedStream* stream_tmp = m_demuxed_stream;
    m_demuxed_stream =
        nullptr;  // обнуляем именно тут, чтобы не было рекурсивного зацикливания при вызове pStreaTmp->UnlinkFromReader();
    stream_tmp->unlink_from_reader();
    /// stream_tmp->release() - вызывать не нужно, т.к. данный метод уже вызван из деструктора этого объекта
}

void StreamReader::StreamInfo::request_seek(TimestampFF time, StreamPtr result_checker)
{
    if (!m_demuxed_stream)
        return;

    // Из-за того, что RequestSeek вызывается по цепочке от наследника к предку, то
    // надо вставлять очередной pResultChecker в начало списка, чтобы получить список объектов в порядке наследования.
    // Т.е. если у нас есть стэк объектов Resize(VideoDecoder(RawStream())),
    // то мы должны проверить результат сначала для RawStream, потом для VideoDecoder, потом для Resize

    // PS: в 99% случаев в векторе будет только 1 объект - декодер для данного потока
    m_seek_already_done = false;

    if (std::find(m_seek_result_checkers.begin(), m_seek_result_checkers.end(), result_checker) ==
        m_seek_result_checkers.end())
    {
        m_seek_result_checkers.insert(m_seek_result_checkers.begin(), result_checker);
    }
    else
    {
        /// убираем эту проверку, потому что один и тот же обработчик может быть добавлен дважды,
        /// если в графе есть ветвления
    }

    if (m_seek_time > time || m_seek_time == AV_NOPTS_VALUE)
        m_seek_time = time;
}

bool StreamReader::StreamInfo::get_seek_result(TimestampFF seek_time)
{
    m_seek_already_done = true;
    bool seek_result = true;
    for (size_t i = 0, count = m_seek_result_checkers.size(); i < count; i++)
    {
        const auto& checker = m_seek_result_checkers[i];
        const bool check_result = checker->get_seek_result();
        if (!check_result)
        {
            seek_result = false;
            if (seek_time > 0)
            {
                /// Когда позиционируемся не в 0, то сразу пропускаем дальнейшие проверки,
                /// чтобы отступить назад и попробовать еще раз
                /// Если позиционируемся в 0, то надо выполнять все проверки, даже если они не удачные.
                /// Это необходимо для того, чтобы проинициализировать все фильтры, т.к. даже после неудачного Seek
                /// вся цепочка будет использоваться.
                /// Пример: делаем seek(0) для файлов mts, у которых временные метки пакетов начинаются не с нуля.
                ///   RawStream::GetSeekResult() вернет false и без этого сами декодеры не будут переинициализированы.
                break;
            }
        }
    }
    return seek_result;
}

TimestampFF StreamReader::StreamInfo::get_seek_position() const { return m_seek_time; }

bool StreamReader::StreamInfo::seek_already_done() const { return m_seek_already_done; }

void StreamReader::StreamInfo::reset_state_before_seek()
{
    m_seek_already_done = false;
    reset_state_after_seek();
}

void StreamReader::StreamInfo::reset_state_after_seek()
{
    m_seek_time = AV_NOPTS_VALUE;
    m_seek_result_checkers.clear();
}

}  // namespace step::video::ff

// StreamReader
namespace step::video::ff {

StreamReader::StreamReader(const DemuxerPtr& demuxer) : m_demuxer(demuxer), m_streams(demuxer->get_stream_count())
{
    for (StreamId i = 0, streamCount = m_demuxer->get_stream_count(); i < streamCount; ++i)
        m_demuxer->enable_stream(i, false);
}

TimeFF StreamReader::get_duration() const { return m_demuxer->get_duration(); }

StreamPtr StreamReader::get_stream(StreamId stream_id)
{
    if (stream_id >= m_streams.size())
        STEP_THROW_RUNTIME("Invalid stream id {}", stream_id);

    if (m_streams[stream_id].is_stream_enabled())
    {
        /// поток уже создан. Запрещено запрашивать поток дважды, в программе надо пользоваться уже созданным объектом
        STEP_THROW_RUNTIME("The stream {} is already exists. It is forbidden to create a stream twice.", stream_id);
    }

    m_demuxer->enable_stream(stream_id, true);
    /// создаем поток недекодированных данный

    /// себе сохраняем сырой указатель, без увеличения счетчика ссылкок,
    /// чтобы удаление потока из StreamReader происходило автоматически
    /// удалении объекта StreamPtr

    auto stream_ptr = std::make_shared<DemuxedStream>(shared_from_this(), stream_id);
    m_streams[stream_id].set_stream(stream_ptr.get());

    /// далее создается умный указатель, со счетчиком ссылок = 1
    return stream_ptr;
}

void StreamReader::release_stream(StreamId stream_id)
{
    if (!m_streams[stream_id].is_stream_enabled())
        return;

    m_demuxer->enable_stream(stream_id, false);
    /// используем сырой указатель, чтобы не наращивать счетчик ссылок
    m_streams[stream_id].unlink_from_reader();
}

int StreamReader::get_stream_count() const { return m_demuxer ? m_demuxer->get_stream_count() : 0; }

int StreamReader::get_active_stream_count() const
{
    int count = 0;
    for (unsigned int i = 0; i < m_streams.size(); i++)
    {
        count += (int)(m_streams[i].is_stream_enabled());
    }
    return count;
}

FormatCodec StreamReader::get_format_codec(StreamId stream_id) const
{
    if (!m_demuxer)
        STEP_THROW_RUNTIME("Can't provide FormatCodec for stream {}: empty demuxer!", stream_id);

    return m_demuxer->get_format_codec(stream_id);
}

StreamPtr StreamReader::get_best_video_stream()
{
    const auto stream_id = m_demuxer->get_best_video_stream_id();
    if (stream_id == INVALID_STREAM_ID)
    {
        STEP_LOG(L_ERROR, "StreamReader can't provide best video stream!");
        return nullptr;
    }

    return get_stream(stream_id);
}

bool StreamReader::is_eof_reached() { return m_demuxer->is_eof_reached(); }

void StreamReader::release_internal_data(StreamId stream_id) { m_demuxer->release_internal_data(stream_id); }

void StreamReader::request_seek(StreamId stream_id, TimestampFF time, const StreamPtr& result_checker)
{
    m_streams[stream_id].request_seek(time, result_checker);
}

void StreamReader::seek(StreamId stream_id)
{
    std::unique_lock lock(m_seek_mutex, std::try_to_lock);
    if (!lock)
    {
        STEP_LOG(L_DEBUG, "StreamReader::seek stream {} is already in seek mode!", stream_id);
        return;
    }

    if ((stream_id >= m_streams.size()) || (!m_streams[stream_id].is_stream_enabled()))
    {
        /// неправильное использование StreamReader-а - явный вызов Seek(..), а не через объект StreamRaw
        STEP_THROW_RUNTIME("Unexpected seek index");
    }

    if (m_streams[stream_id].seek_already_done())
    {
        /// Seek уже выполнен. Ничего не делаем.
        m_streams[stream_id].reset_state_before_seek();
        return;
    }

    /// ищем минимальную из запрошенных временных меток
    TimestampFF seek_pos = AV_NOPTS_VALUE;
    for (unsigned int i = 0; i < m_streams.size(); i++)
    {
        if (!m_streams[i].is_stream_enabled())
            continue;

        TimestampFF stream_seek_pos = m_streams[i].get_seek_position();
        if (stream_seek_pos == AV_NOPTS_VALUE)
        {
            /// Метод DoSeek() был вызван до вызова RequestSeek(..) для всех потоков, которые были созданы текущим IStreamReader
            STEP_THROW_RUNTIME("Unexpected seek");
        }
        if (seek_pos == AV_NOPTS_VALUE || seek_pos > stream_seek_pos)
        {
            /// ищем наименьшую временную метку, на которую надо делать Seek
            seek_pos = stream_seek_pos;
        }
    }

    const TimeFF MAX_STEP = 16 * AV_SECOND;
    const TimeFF MAX_TIME_SHIFT = 60 * AV_SECOND;

    TimeFF seek_timeshift = 0;
    const TimestampFF start_seek_pos = seek_pos;
    while (seek_pos >= 0)
    {
        /// отступ назад для повторного Seek
        if (seek_pos >= seek_timeshift && (start_seek_pos - seek_pos < MAX_TIME_SHIFT))
        {
            seek_pos -= seek_timeshift;
            if (seek_timeshift < MAX_STEP)  /// отступ назад не более, чем 16 сек за раз
            {
                /// наращивание шага для ускорения поиска
                seek_timeshift = seek_timeshift ? seek_timeshift * 2 : AV_SECOND;
            }
        }
        else
            seek_pos = 0;

        TimestampFF demuxer_pos = m_demuxer->seek(seek_pos);
        if (demuxer_pos < seek_pos)
        {
            /// Оптимизация: принудительно меняем на то значение, которое использовал демуксер
            seek_pos = demuxer_pos;
        }

        /// проверяем результаты Seek
        bool seek_result = true;
        for (unsigned int i = 0; i < m_streams.size(); ++i)
        {
            StreamInfo& info = m_streams[i];
            if (!info.get_seek_result(seek_pos))
            {
                seek_result = false;
                if (seek_pos > 0)
                {
                    /// если есть возможность отодвинуться назад,
                    /// то останавливаем проверку результата seek
                    /// и отодвигаемся назад
                    break;

                    /// если мы делаем Seek(0), то прерывать цикл нельзя,
                    /// т.к. надо проинициализировать все потоки после Seek,
                    /// а это делается вызовом GetSeekResult()
                }
            }
        }
        if (seek_result)
        {
            /// позиционирование выполнено успешно
            STEP_LOG(L_DEBUG, "StreamReader::Seek {} = OK", seek_pos);
            break;
        }
        STEP_LOG(L_DEBUG, "StreamReader::Seek {} = FAILED", seek_pos);
        if (!seek_pos)
        {
            /// ошибка - не получилось сделать seek, т.к. какие-то из потоков
            /// возвращали get_seek_result()==false даже при seek(0)
            /// тут ничего не сделаешь - такой файл. Будем отдавать пакеты с тех позиций, которые мы нашли.
            STEP_LOG(L_ERROR, "StreamReader::Seek {} - seek error", seek_pos);
            break;
        }
    }

    for (unsigned int i = 0; i < m_streams.size(); i++)
        m_streams[i].reset_state_after_seek();

    m_streams[stream_id].reset_state_before_seek();  ///< Помечаем, что позиционирование по данному потоку выполнено
}

}  // namespace step::video::ff