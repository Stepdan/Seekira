#include "ff_demuxer_queue.hpp"

#include <core/log/log.hpp>

namespace step::video::ff {

DemuxerQueue::DemuxerQueue(const std::shared_ptr<ParserFF>& parser)
    : m_parser(parser), m_working_time(0), m_processed_count(0)
{
    m_streams = m_parser->get_stream_count();
    for (StreamId i = 0; i < m_streams; i++)
    {
        std::shared_ptr<PacketQueue> elem = std::make_shared<PacketQueue>(m_parser->get_stream_type(i));
        m_queues.push_back(elem);
    }

    m_key_packets_must_exist = true;
    m_seek_time = 0;
    m_eof_is_reached = false;
}

void DemuxerQueue::close() {}

DemuxerQueue::~DemuxerQueue() { close(); }

TimeFF DemuxerQueue::get_duration() const { return m_parser->get_duration(); }

TimeFF DemuxerQueue::get_stream_duration(StreamId stream) const { return m_parser->get_stream_duration(stream); }

int DemuxerQueue::get_stream_count() const { return m_streams; }

MediaType DemuxerQueue::get_stream_type(StreamId index) const { return m_parser->get_stream_type(index); }

TimestampFF DemuxerQueue::seek(TimestampFF time)
{
    STEP_LOG(L_DEBUG, "=============================================");
    STEP_LOG(L_DEBUG, "Seek {}", time);

    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    m_seek_time = (std::max)(TimestampFF(0), time);  ///< фиксируем начальную временную метку

    const TimestampFF WARN_STEP = 20 * AV_SECOND;

    TimestampFF step = AV_SECOND * 4;
    bool res = false;
    while (time >= 0)
    {
        res = seek_internal(time);
        if (res || time == 0)
            break;
        time = time < step ? 0 : time - step;
        step *= 2;
        if (step > WARN_STEP)
            STEP_LOG(L_WARN, "Exceeded warn step: {}", step);
    }
    STEP_LOG(L_DEBUG, "Seek {}, Result {}", time, (int)res);
    return time;
}

bool DemuxerQueue::seek_internal(TimestampFF time)
{
    StreamId idx_start, idx_stop;
    if (time > 0)
    {
        idx_start = m_parser->get_seek_stream();
        idx_stop = idx_start + 1;
    }
    else
    {
        idx_start = 0;
        idx_stop = m_streams;
    }

    std::vector<TimestampFF> seek_positions(
        m_streams,
        AV_NOPTS_VALUE);  // seek_positions[] - наименьшие позиции потоков после seek
    StreamId best_index = UINT32_MAX;
    StreamId last_index = 0;
    for (StreamId idx = idx_start; idx < idx_stop; idx++)
    {
        STEP_LOG(L_DEBUG, "Seek {} / {} (id/streams), time {}", idx, m_streams, time);
        MediaType media_type = m_parser->get_stream_type(idx);
        if (media_type != MediaType::Audio && media_type != MediaType::Video)
        {
            STEP_LOG(L_DEBUG, "Stream {} is not video or audio, it can't be used for seek - SKIPPED");
            continue;
        }
        reset_queue();
        last_index = idx;
        if (best_index == UINT32_MAX)
        {
            best_index = last_index;
        }
        try
        {
            m_parser->seek(last_index, time);
        }
        catch (std::exception& e)
        {
            STEP_LOG(L_ERROR, "Handled exception: seek. {}", e.what());
            continue;
        }
        catch (...)
        {
            STEP_LOG(L_ERROR, "Handled unknown exception: seek.");
            continue;
        }

        /// Быстрая проверка, что мы находимся в правильном месте в файле:
        /// пытаемся заполнить очередь так, чтобы в каждом потоке был хотя бы один пакет.
        /// Такое заполнение работает быстро.
        fill_queue(false);

        /// А теперь проверяем временные метки пакетов, они должны быть меньше запрошенной позиции
        const bool seek_position_is_valid = check_streams_position(m_seek_time);
        if (seek_position_is_valid)
        {
            /// Если это так, то дозаполняем очередь, чтобы везде было по 2 ключевых пакета.
            /// 2 ключевых пакета в очереди гарантируют, что первый пакет будет ДО запрошенной ключевой метки,
            /// а второй - после
            fill_queue(true);
            /// return;  <<< тут return не нужен, т.к. надо проверить результат Seek
        }
        /// проверяем, не улучшились ли временные метки во время этого этапа позиционирования
        bool seek_is_better = true;
        for (StreamId i = 0; i < m_streams; i++)
        {
            const PacketQueue& queue = *m_queues[i];
            if (queue.is_enabled())
            {
                TimestampFF pos = queue.position();
                if (seek_positions[i] != AV_NOPTS_VALUE)
                {
                    if (pos != AV_NOPTS_VALUE)
                    {
                        if (pos > seek_positions[i])
                        {
                            seek_is_better = false;
                        }
                    }
                    else
                    {
                        seek_is_better = false;
                    }
                }
            }
        }

        if (seek_is_better)
        {
            STEP_LOG(L_DEBUG, "Seek is better than before");

            /// сохраняем номер потока, на котором получили улучшенный результат
            best_index = idx;

            bool seek_is_done = true;
            for (StreamId i = 0; i < m_streams; i++)
            {
                const PacketQueue& queue = *m_queues[i];
                if (queue.is_enabled())
                {
                    /// сохраняем улучшенные значения
                    seek_positions[i] = queue.position();
                    MediaType media_type = m_parser->get_stream_type(i);

                    if ((media_type == MediaType::Audio) || (media_type == MediaType::Video))
                    {
                        if (seek_positions[i] != AV_NOPTS_VALUE)
                        {
                            seek_is_done &= (seek_positions[i] <= m_seek_time);
                        }
                        else
                        {
                            seek_is_done = false;
                        }
                    }
                    else
                    {
                        /// для других типов потоков нам не важно наличие пакетов в очереди
                    }
                }
            }

            if (seek_is_done)
            {
                STEP_LOG(L_DEBUG, "Seek is OK");
                /// позиционирование сделали
                return true;
            }
        }
    }

    if (best_index == UINT32_MAX)
        STEP_THROW_RUNTIME("Seek can't be done, file is corrupted or not seekable");

    if (time == 0)
    {
        if (best_index != last_index)
        {
            STEP_LOG(L_DEBUG, "Making a seek using best index {}", best_index);

            reset_queue();
            m_parser->seek(best_index, time);
            fill_queue(true);

            /// вывод позиций в лог
            check_streams_position(time);
        }
        return true;
    }

    return false;
}

bool DemuxerQueue::is_eof_reached() { return m_eof_is_reached; }

void DemuxerQueue::reset_queue()
{
    for (StreamId i = 0; i < m_streams; i++)
    {
        m_queues[i]->reset();
    }
    // заполнение очереди так, чтобы всегда был хотя бы один ключевой пакет
    m_key_packets_must_exist = true;
    m_eof_is_reached = false;
}

// читает из файла iPacketCount пакетов,
// при этом чистит очереди, если это необходимо
// например, в режиме Seek удаляет ненужные пакеты,
// которые имеют меньшую временную метку, чем SeekTime:
// удаление идет от ключевого до ключевого
bool DemuxerQueue::read_packets_from_parser(int packetCount)
{
    if (m_eof_is_reached)
    {
        return false;
    }

    for (int z = 0; z < packetCount; z++)
    {
        std::shared_ptr<IDataPacket> data_packet = m_parser->read();
        if (!data_packet)
        {
            m_eof_is_reached = true;
            return false;
        }
        const StreamId stream_index = data_packet->get_stream_index();
        const TimestampFF pts_time = data_packet->pts();
        const TimestampFF dts_time = data_packet->dts();
        const TimeFF duration = data_packet->duration();
        const bool key_frame = data_packet->is_key_frame();
        PacketQueue& queue = *m_queues[stream_index];
        if (!queue.is_enabled())
        {
            queue.reset();  // очистка очереди
            z--;
            continue;
        }
        // если пришел ключевой пакет, а его временная метка меньше, чем SeekTime
        // то все предыдущие пакеты можно удалить - они не нужны
        bool resetQueue = false;
        if (key_frame)
        {
            if (pts_time != AV_NOPTS_VALUE)
            {
                resetQueue = (pts_time <= m_seek_time) && (duration > 0);
            }
            else if (dts_time != AV_NOPTS_VALUE)
            {
                resetQueue = (dts_time <= m_seek_time) && (duration > 0);
            }

            if (resetQueue)  // удаляем пакеты!!!
            {
                queue.reset();
            }
        }

        STEP_LOG(L_TRACE, "StreamId {} {} {} PTS {}, DTS {}, Duration {}, Size {}", stream_index,
                 (key_frame ? "+: " : " : "), (reset_queue ? "Reset, " : ""), pts_time, dts_time, duration,
                 data_packet->size());

        queue.push(data_packet);
        // проверка на переполнение очереди
        if (queue.is_full())
        {
            return false;
        }
    }
    return true;
}

/// Проверяет, что позиции всех требуемых потоков не превышают заданную временную метку
/// Используется для проверки позиций после Seek
/// Возвращает: true - пакеты по всех потоках есть, и их позиции меньше maxAllowedPosition
///             false - иначе
bool DemuxerQueue::check_streams_position(TimestampFF max_allowed_position)
{
    STEP_LOG(L_DEBUG, "Checking stream positions (must be less than {}", max_allowed_position);

    bool seek_is_good = true;
    for (StreamId i = 0; i < m_streams; i++)
    {
        const PacketQueue& queue = *m_queues[i];
        if (queue.is_enabled() && !queue.can_be_empty())
        {
            TimestampFF pos = queue.position();
            bool res = (pos != AV_NOPTS_VALUE) && (pos <= max_allowed_position);
            seek_is_good &= res;
            STEP_LOG(L_DEBUG, "Stream {}: pos {} (delta {}) - {}", i, pos, (pos - max_allowed_position),
                     (res ? "ok" : "error"));
        }
    }
    return seek_is_good;
}

// Заполнение очереди пакетами.
// Если key_packets_must_exist=true - это заполнение после Seek, чтобы в каждой очереди был хотя бы 1 ключевой пакет
// Если key_packets_must_exist=false - обычное заполнение, чтобы в каждой очереди был хотя бы 1 пакет
bool DemuxerQueue::fill_queue(bool key_packets_must_exist)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    if (m_eof_is_reached)
    {
        return false;
    }

    bool need_fill = false;  // true = хотя бы одна из включенных очередей пуста
    bool queue_is_full = false;  // true = признак того, что какая-то очередь переполнена
    for (StreamId i = 0; i < m_streams; i++)
    {
        const PacketQueue& queue = *m_queues[i];
        int size = queue.size();
        need_fill = need_fill || (queue.is_enabled() &&
                                  ((size == 0) || (key_packets_must_exist && (queue.get_key_frame_count() < 2))));
        queue_is_full = queue_is_full || queue.is_full();
    }

    if (queue_is_full)
    {
        //ничего заполнять не надо
        return false;
    }

    while (need_fill && !queue_is_full)
    {
        // заполнение по 16 пакетов
        bool read_result = read_packets_from_parser(16);

        if (!read_result && m_eof_is_reached)
        {
            int total_packets = 0;
            for (StreamId i = 0; i < m_streams; i++)
            {
                total_packets += m_queues[i]->size();
            }
            if (!total_packets)
                return false;
            return true;
        }
        need_fill = false;
        queue_is_full = false;
        if (read_result)
        {
            for (StreamId i = 0; i < m_streams; i++)
            {
                const PacketQueue& queue = *m_queues[i];
                STEP_LOG(L_TRACE, "Stream {}: packets {} (keyframes {})", i, queue.size(), queue.get_key_frame_count());
                if (!key_packets_must_exist)
                {
                    need_fill = need_fill || (queue.is_enabled() && (queue.size() == 0) && !queue.can_be_empty());
                }
                else
                {
                    need_fill =
                        need_fill || (queue.is_enabled() && (queue.get_key_frame_count() < 2) && !queue.can_be_empty());
                }
                queue_is_full = queue_is_full || queue.is_full();
            }
        }
    }
    return true;
}

std::shared_ptr<IDataPacket> DemuxerQueue::read(StreamId stream)
{
    //MAKE_TIMER(m_working_time);
    if (stream >= m_streams)
        STEP_THROW_RUNTIME("Stream with provided index {} doesn't exist", stream);

    // если надо, то заполняем очередь
    PacketQueue& queue = *m_queues[stream];
    bool no_packets = (queue.size() == 0);
    if (no_packets)
    {
        // если в очереди для текущего потока нет пакетов,
        // то идет заполнение очередей так, чтобы в каждой очереди был хотя бы один пакет
        fill_queue(false);
        no_packets = (queue.size() == 0);
    }

    if (no_packets)
    {
        return nullptr;
    }
    auto packet = queue.pop();
    m_processed_count += (bool)packet;
    return packet;
}

void DemuxerQueue::enable_stream(StreamId stream, bool value)
{
    if (stream >= m_streams)
        STEP_THROW_RUNTIME("Stream with provided index {} doesn't exist", stream);

    /// если value == false, то очередь будет очищена
    m_queues[stream]->set_enabled(value);
}

void DemuxerQueue::release_internal_data(StreamId stream) { m_queues[stream]->reset(); }

}  // namespace step::video::ff