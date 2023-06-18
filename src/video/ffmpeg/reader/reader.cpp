#include "reader.hpp"

#include <core/log/log.hpp>
#include <core/exception/assert.hpp>

#include <core/base/utils/string_utils.hpp>

#include <proc/drawer/drawer.hpp>
#include <proc/interfaces/detector_interface.hpp>
#include <proc/detect/registrator.hpp>
#include <proc/settings/settings_face_detector.hpp>

namespace {

constexpr size_t MAX_INVALID_THRESHOLD = 10;

}  // namespace

namespace step::video::ff {

ReaderFF::ReaderFF(ReaderMode mode) : m_mode(mode) {}

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

    set_reader_state(ReaderState::Reading);

    m_filename = filename;
    m_invalid_counter = 0;

    return true;
}

void ReaderFF::start()
{
    if (is_running())
    {
        STEP_LOG(L_WARN, "Reader already started");
        return;
    }

    m_continue_reading.store(false);
    set_reader_state(ReaderState::Reading);

    run_worker();

    STEP_LOG(L_INFO, "Start ReaderFF");
}

TimeFF ReaderFF::get_duration() const
{
    STEP_ASSERT(m_stream, "ReaderFF can't provide duration: empty stream!");
    return m_stream->get_duration();
}

TimestampFF ReaderFF::get_position() const
{
    STEP_ASSERT(m_last_valid_frame, "ReaderFF can't provide position: invalid last frame!");
    return m_last_valid_frame->ts.count();
}

ReaderState ReaderFF::get_state() const { return m_state; }

void ReaderFF::add_reader_event(ReaderEvent event)
{
    std::scoped_lock lock(m_event_guard);
    m_event_queue.push(event);
    STEP_LOG(L_INFO, "Reader event has been queued: {}", step::utils::to_string(event.get_type()));
    m_read_cnd.notify_all();
}

bool ReaderFF::has_event() const
{
    std::scoped_lock lock(m_event_guard);
    return !m_event_queue.empty();
}

void ReaderFF::handle_event(ReaderEvent event)
{
    switch (event.get_type())
    {
        case ReaderEvent::Type::Pause:
            pause_impl();
            break;
        case ReaderEvent::Type::Play:
            play_impl();
            break;
        case ReaderEvent::Type::RewindBackward:
            rewind_backward_impl();
            break;
        case ReaderEvent::Type::RewindForward:
            rewind_forward_impl();
            break;
        case ReaderEvent::Type::SetPosition:
            set_position_impl(event.get_timestamp());
            break;
        case ReaderEvent::Type::StepBackward:
            step_backward_impl();
            break;
        case ReaderEvent::Type::StepForward:
            step_forward_impl();
            break;
        case ReaderEvent::Type::Stop:
            stop_impl();
            break;
        default:
            STEP_UNDEFINED("Invalid reader event type");
            break;
    }

    m_read_cnd.notify_all();
}

/* clang-format off */
void ReaderFF::play             () { add_reader_event(ReaderEvent(ReaderEvent::Type::Play           )); }
void ReaderFF::pause            () { add_reader_event(ReaderEvent(ReaderEvent::Type::Pause          )); }
void ReaderFF::stop             () { add_reader_event(ReaderEvent(ReaderEvent::Type::Stop           )); }
void ReaderFF::step_forward     () { add_reader_event(ReaderEvent(ReaderEvent::Type::StepForward    )); }
void ReaderFF::step_backward    () { add_reader_event(ReaderEvent(ReaderEvent::Type::StepBackward   )); }
void ReaderFF::rewind_forward   () { add_reader_event(ReaderEvent(ReaderEvent::Type::RewindForward  )); }
void ReaderFF::rewind_backward  () { add_reader_event(ReaderEvent(ReaderEvent::Type::RewindBackward )); }

void ReaderFF::set_position(TimestampFF ts) { add_reader_event(ReaderEvent(ReaderEvent::Type::SetPosition, ts)); }
/* clang-format on */

void ReaderFF::set_reader_state(ReaderState state)
{
    m_state = state;
    m_reader_observers.perform_for_each_event_handler(
        std::bind(&IReaderEventObserver::on_reader_state_changed, std::placeholders::_1, m_state));
}

void ReaderFF::seek(TimestampFF pos)
{
    m_stream->request_seek(pos, nullptr);
    m_stream->do_seek();
    if (!m_stream->get_last_seek_result())
    {
        STEP_LOG(L_ERROR, "Invalid seek to {}", pos);
        set_reader_state(ReaderState::Error);
        return;
    }

    STEP_LOG(L_DEBUG, "Succesful seek to {} [{}]", m_stream->get_position(), pos);
}

void ReaderFF::read_frame()
{
    auto frame_ptr = m_stream->read_frame();

    m_is_last_key_frame = m_stream->is_last_key_frame();
    const bool need_handle = need_handle_frame();

    if (frame_ptr)
    {
        m_invalid_counter = 0;
        m_prev_duration = m_last_valid_frame ? m_last_valid_frame->duration : 0;
        m_last_valid_frame = frame_ptr;

        if (need_handle)
        {
            reader_process_frame(frame_ptr);
            m_frame_observers.perform_for_each_event_handler(
                std::bind(&IFrameSourceObserver::process_frame, std::placeholders::_1, frame_ptr));
        }
    }
    else
    {
        ++m_invalid_counter;
        STEP_LOG(L_WARN, "Read invalid frame, count {}", m_invalid_counter);
    }

    STEP_LOG(L_DEBUG, "Read {} frame ({}handled) with ts {}, duration {}", frame_ptr ? "valid" : "invalid",
             need_handle ? "" : "not ", m_last_valid_frame->ts.count(), m_last_valid_frame->duration);
}

bool ReaderFF::need_break_reading(bool verbose /*= false*/) const
{
    bool need_break = false;

    need_break = need_break || m_need_stop;

    /* clang-format off */
    const auto is_valid_state = true
        && m_state != ReaderState::Error
        && m_state != ReaderState::Stopped
        && m_state != ReaderState::Undefined
    ;
    /* clang-format on */

    need_break = need_break || !is_valid_state;

    const auto invalid_threshold = m_invalid_counter > MAX_INVALID_THRESHOLD;
    need_break = need_break || invalid_threshold;

    if (verbose)
    {
        STEP_LOG(L_INFO, "Check need break: state {}, invalid counter {}", step::utils::to_string(m_state),
                 m_invalid_counter);
    }

    return need_break;
}

bool ReaderFF::need_handle_frame()
{
    if (m_skip_frame_handle)
        return false;

    /* clang-format off */
    return false
        || m_mode == ReaderMode::All
        || m_is_last_key_frame
        || m_need_handle_after_force_set_pos
    ;
    /* clang-format on */
}

/* clang-format off */
void ReaderFF::register_observer(IFrameSourceObserver* observer) { m_frame_observers.register_event_handler(observer); }
void ReaderFF::register_observer(IReaderEventObserver* observer) { m_reader_observers.register_event_handler(observer); }
void ReaderFF::unregister_observer(IFrameSourceObserver* observer) { m_frame_observers.unregister_event_handler(observer); }
void ReaderFF::unregister_observer(IReaderEventObserver* observer) { m_reader_observers.unregister_event_handler(observer); }
/* clang-format on */

}  // namespace step::video::ff

namespace step::video::ff {

void ReaderFF::worker_thread()
{
    set_reader_state(ReaderState::Reading);
    while (!need_break_reading())
    {
        try
        {
            std::unique_lock lock(m_read_guard);

            {  // Если есть необработанный эвент - обрабатываем, кадр не читаем
                std::scoped_lock event_lock(m_event_guard);
                if (!m_event_queue.empty())
                {
                    auto event = m_event_queue.back();
                    m_event_queue.pop();
                    handle_event(event);
                    continue;
                }
            }

            if (m_stream->is_eof_reached())
                m_continue_reading = false;

            if (!m_continue_reading)
                set_reader_state(ReaderState::Paused);

            m_read_cnd.wait(lock, [this]() { return need_break_reading() || has_event() || m_continue_reading; });

            if (need_break_reading())
                break;

            if (has_event())
                continue;

            set_reader_state(ReaderState::Reading);
            read_frame();
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

    m_continue_reading.store(false);

    if (!m_need_stop && need_break_reading())
    {
        set_reader_state(ReaderState::Error);
        STEP_LOG(L_ERROR, "ReaderFF worker thread finished by error! State {}", step::utils::to_string(m_state));
        return;
    }

    if (m_state == ReaderState::EndOfFile)
    {
        pause();
    }

    STEP_LOG(L_INFO, "ReaderFF worker thread finished!");
}

void ReaderFF::thread_worker_stop_impl() {}

}  // namespace step::video::ff