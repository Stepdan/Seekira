#include "reader.hpp"

#include <core/log/log.hpp>

#include <core/base/utils/string_utils.hpp>

namespace step::video::ff {

ReaderFF::ReaderFF() : m_need_read_cnd(false) {}

ReaderFF::~ReaderFF()
{
    STEP_LOG(L_INFO, "ReaderFF destruction, file: {}", m_filename);
    stop();
}

bool ReaderFF::open_file(const std::string& filename)
{
    m_parser = std::make_shared<ParserFF>();
    if (!m_parser->open_file(filename))
    {
        STEP_LOG(L_ERROR, "Failed to open file {}", filename);
        return false;
    }

    m_demuxer = std::make_shared<DemuxerQueue>(m_parser);
    m_stream_reader = std::make_shared<StreamReader>(m_demuxer);

    m_stream = m_stream_reader->get_best_video_stream();

    set_reader_state(ReaderState::Ready);

    m_filename = filename;

    return true;
}

TimeFF ReaderFF::get_duration() const
{
    STEP_ASSERT(m_stream, "ReaderFF can't provide duration: empty stream!");
    return m_stream->get_duration();
}

TimeFF ReaderFF::get_frame_duration() const
{
    STEP_ASSERT(m_last_frame_duration != AV_NOPTS_VALUE,
                "ReaderFF can't provide frame duration: m_last_frame_duration is empty!");
    return m_last_frame_duration;
}

TimestampFF ReaderFF::get_position() const
{
    STEP_ASSERT(m_stream, "ReaderFF can't provide position: empty stream!");
    return m_last_frame_ts;
}

ReaderState ReaderFF::get_state() const { return m_state; }

void ReaderFF::start(ReadingMode mode)
{
    if (mode == ReadingMode::Undefined)
    {
        STEP_LOG(L_ERROR, "Can't start reading: invalid mode!");
        return;
    }

    m_read_mode = mode;
    run_worker();

    bool is_continuously = m_read_mode == ReadingMode::Continuously;

    set_reader_state(is_continuously ? ReaderState::ReadingContiniously : ReaderState::ReadingByRequest);
    m_need_read_cnd.set_value(is_continuously);

    if (is_continuously)
        m_need_read_cnd.notify_one();

    STEP_LOG(L_INFO, "Start ReaderFF with params: state {}, mode {}", step::utils::to_string(m_state),
             step::utils::to_string(m_read_mode));
}

void ReaderFF::stop()
{
    stop_worker();

    // m_need_read_cnd.notify will be done in thread_worker_stop_impl()
    // after m_need_stop.store(true) in ThreadWorker::stop()

    // release request_read
    m_need_read_cnd.set_value(false);
    m_request_read_finished_cnd.notify_all();

    m_state = ReaderState::Ready;
}

void ReaderFF::set_position(TimestampFF pos)
{
    STEP_LOG(L_DEBUG, "Try to set position {} to {}", get_position(), pos);

    // Фиксим выходы за временные границы
    if (pos < 0)
        pos = 0;

    if (pos > get_duration())
        pos = get_duration();

    const auto prev_read_mode = m_read_mode;

    // Отключаем обзерверов, чтобы не летели кадры и статусы
    m_frame_observers.disable();
    m_reader_observers.disable();

    // TODO Краевые условия

    stop();

    seek(pos);

    if (m_state != ReaderState::SuccessfulSeek)
        return;

    // После seek мы стоим на ключевом кадре, надо дочитать до нужного места, если требуется

    // Читаем по одному кадру, пока следующий для чтения кадр не будет нужным
    start(ReadingMode::ByRequest);
    auto next_frame_ts = m_stream->get_position();
    // duration следующего кадра: m_stream->get_pkt_duration();
    auto after_next_frame_ts = next_frame_ts + m_stream->get_pkt_duration();

    // Проверяем, что дошли по таймштампам до нужного кадра (следующий при чтении - нужный)
    const auto check_ts = [&pos, &next_frame_ts, &after_next_frame_ts]() {
        return next_frame_ts != pos && after_next_frame_ts < pos;
    };

    // Состояние должно быть валидным после чтения кадра
    const auto check_state = [this]() {
        return m_state == ReaderState::ReadingByRequest || m_state == ReaderState::SuccessfulSeek;
    };

    while (check_state() && check_ts())
    {
        request_read();
        next_frame_ts = m_stream->get_position();
        after_next_frame_ts = next_frame_ts + m_stream->get_pkt_duration();
    }

    // Дошли до нужного места (или не дошли из-за ошибки) - включаем обзерверов
    m_frame_observers.enable();
    m_reader_observers.enable();

    if (!check_state())
    {
        STEP_LOG(L_DEBUG, "Invalid set_position to {} because of invalid state: {}", pos,
                 step::utils::to_string(m_state));
        // перезапишем состояние, чтобы оно улетело подписчикам
        set_reader_state(m_state);
        return;
    }

    // Читаем нужный нам кадр
    STEP_LOG(L_DEBUG, "Read frame with desired position");
    request_read();

    STEP_LOG(L_DEBUG, "Position has been setted to {} [{}]", m_last_frame_ts, pos);

    // Если предыдущий режим был Continiously - принудительно включаем
    if (prev_read_mode == ReadingMode::Continuously)
    {
        stop();
        start(ReadingMode::Continuously);
    }
}

void ReaderFF::seek(TimestampFF pos)
{
    set_reader_state(ReaderState::TryToSeek);
    m_stream->request_seek(pos, nullptr);
    m_stream->do_seek();
    if (!m_stream->get_last_seek_result())
    {
        STEP_LOG(L_ERROR, "Invalid seek to {}", pos);
        set_reader_state(ReaderState::InvalidSeek);
        return;
    }

    STEP_LOG(L_INFO, "Succesful seek to {} [{}]", m_stream->get_position(), pos);
    set_reader_state(ReaderState::SuccessfulSeek);
}

void ReaderFF::request_read()
{
    std::unique_lock lock(m_read_guard);

    if (m_read_mode != ReadingMode::ByRequest)
    {
        STEP_LOG(L_ERROR, "Can't do request_read: read mode is {}", step::utils::to_string(m_read_mode));
        return;
    }

    if (m_state != ReaderState::ReadingByRequest && m_state != ReaderState::SuccessfulSeek)
    {
        STEP_LOG(L_WARN, "Can't do request_read: reader state is {}", step::utils::to_string(m_state));
        return;
    }

    if (m_need_read_cnd.get_value())
    {
        STEP_THROW_RUNTIME("request_read has m_need_read = true instead of false");
    }

    STEP_LOG(L_TRACE, "Request read");
    m_need_read_cnd.set_value(true);
    m_need_read_cnd.notify_all();

    // Wait until reading finish
    m_request_read_finished_cnd.wait(lock, [this]() { return !m_need_read_cnd.get_value(); });

    STEP_LOG(L_DEBUG, "Read frame with ts {}, duration {}", m_last_frame_ts, m_last_frame_duration);
}

void ReaderFF::request_read_prev()
{
    if (m_read_mode != ReadingMode::ByRequest)
    {
        STEP_LOG(L_ERROR, "Can't do request_read_prev: read mode is {}", step::utils::to_string(m_read_mode));
        return;
    }

    if (m_state != ReaderState::ReadingByRequest && m_state != ReaderState::SuccessfulSeek)
    {
        STEP_LOG(L_WARN, "Can't do request_read_prev: reader state is {}", step::utils::to_string(m_state));
        return;
    }

    if (m_need_read_cnd.get_value())
    {
        STEP_THROW_RUNTIME("request_read_prev has m_need_read = true instead of false");
    }

    if (m_last_frame_duration == AV_NOPTS_VALUE)
    {
        STEP_LOG(L_ERROR, "Can't do request_read_prev: m_last_frame_duration is invalid");
        return;
    }

    // TODO recursive lock?
    set_position(get_position() - m_last_frame_duration);
}

void ReaderFF::set_reader_state(ReaderState state)
{
    m_state = state;
    m_reader_observers.perform_for_each_event_handler(
        std::bind(&IReaderEventObserver::on_reader_state_changed, std::placeholders::_1, m_state));
}

void ReaderFF::run_worker() { ThreadWorker::run_worker(); }

void ReaderFF::stop_worker() { ThreadWorker::stop_worker(); }

void ReaderFF::thread_worker_stop_impl()
{
    // m_need_stop already 'true' (see ThreadWorker::stop())
    m_need_read_cnd.notify_all();
}

void ReaderFF::worker_thread()
{
    const auto break_predicate = [this]() { return m_need_stop || m_stream->is_eof_reached(); };

    while (!break_predicate())
    {
        try
        {
            m_need_read_cnd.wait(
                [this, &break_predicate]() { return break_predicate() || m_need_read_cnd.get_value(); });

            {
                if (break_predicate())
                {
                    // Освобождаем "ждуна" в request_read
                    m_request_read_finished_cnd.notify_all();
                    break;
                }

                auto frame_ptr = m_stream->read_frame();
                if (frame_ptr)
                {
                    m_last_frame_ts = frame_ptr->ts.count();
                    m_last_frame_duration = frame_ptr->duration;

                    m_frame_observers.perform_for_each_event_handler(
                        std::bind(&IFrameSourceObserver::process_frame, std::placeholders::_1, frame_ptr));

                    // In case of ByRequest we try to obtain valid frame.
                    // If there are no valid frames - Error or Break or EOF occures
                    if (m_read_mode == ReadingMode::ByRequest)
                    {
                        m_need_read_cnd.set_value(false);
                        m_request_read_finished_cnd.notify_all();
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            STEP_LOG(L_ERROR, "Failed read frame: {}", e.what());
            m_exception_ptr = std::current_exception();
            break;
        }
        catch (...)
        {
            STEP_LOG(L_ERROR, "Unhandled exception during read frame");
            m_exception_ptr = std::current_exception();
            break;
        }
    }

    // Smth happened during read, reset need_read flag, no valid frame here
    if (m_read_mode == ReadingMode::ByRequest)
    {
        STEP_LOG(L_DEBUG, "Set need_read to 'false' during finishing ReaderFF thread worker");
        m_need_read_cnd.set_value(false);
        m_request_read_finished_cnd.notify_all();
    }

    if (!m_need_stop && m_exception_ptr)
        set_reader_state(ReaderState::Error);

    if (!m_need_stop && m_stream->is_eof_reached())
        set_reader_state(ReaderState::EndOfFile);

    STEP_LOG(L_INFO, "ReaderFF worker thread finished!");
}

/* clang-format off */
void ReaderFF::register_observer(IFrameSourceObserver* observer) { m_frame_observers.register_event_handler(observer); }
void ReaderFF::register_observer(IReaderEventObserver* observer) { m_reader_observers.register_event_handler(observer); }
void ReaderFF::unregister_observer(IFrameSourceObserver* observer) { m_frame_observers.unregister_event_handler(observer); }
void ReaderFF::unregister_observer(IReaderEventObserver* observer) { m_reader_observers.unregister_event_handler(observer); }
/* clang-format on */

}  // namespace step::video::ff