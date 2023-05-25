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

void ReaderFF::open_file(const std::string& filename)
{
    m_parser = std::make_shared<ParserFF>();
    m_parser->open_file(filename);

    m_demuxer = std::make_shared<DemuxerQueue>(m_parser);
    m_stream_reader = std::make_shared<StreamReader>(m_demuxer);

    m_stream = m_stream_reader->get_best_video_stream();

    set_reader_state(ReaderState::Ready);

    m_filename = filename;
}

TimeFF ReaderFF::get_duration() const
{
    if (!m_stream)
        STEP_THROW_RUNTIME("ReaderFF can't provide duration: empty stream!");

    return m_stream->get_duration();
}

ReaderState ReaderFF::get_state() const
{
    std::scoped_lock lock(m_read_guard);
    return m_state;
}

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

    // m_need_read_cnd.notify will be done in before_thread_worker_join()
    // after m_need_stop.store(true) in ThreadWorker::stop()

    // release request_read
    m_need_read_cnd.set_value(false);
    m_request_read_finished_cnd.notify_all();
}

void ReaderFF::seek(TimestampFF pos)
{
    std::scoped_lock lock(m_read_guard);

    STEP_LOG(L_INFO, "Try to seek to {}", pos);
    const auto prev_state = m_state;
    set_reader_state(ReaderState::TryToSeek);

    stop();

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

    if (prev_state == ReaderState::ReadingContiniously || prev_state == ReaderState::ReadingByRequest)
    {
        start(m_read_mode);
    }
}

void ReaderFF::request_read()
{
    std::unique_lock lock(m_read_guard);

    if (m_read_mode != ReadingMode::ByRequest)
    {
        STEP_LOG(L_ERROR, "Can't do request_read: read mode is {}", step::utils::to_string(m_read_mode));
        return;
    }

    if (m_state != ReaderState::ReadingByRequest)
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
}

void ReaderFF::set_reader_state(ReaderState state)
{
    m_state = state;
    m_reader_observers.perform_for_each_event_handler(
        std::bind(&IReaderEventObserver::on_reader_state_changed, std::placeholders::_1, m_state));
}

void ReaderFF::run_worker() { ThreadWorker::run_worker(); }

void ReaderFF::stop_worker() { ThreadWorker::stop_worker(); }

void ReaderFF::before_thread_worker_join()
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
                    break;

                auto frame_ptr = m_stream->read_frame();
                if (frame_ptr)
                {
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
        STEP_LOG(L_INFO, "Set need_read to 'false' during finishing ReaderFF thread worker");
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