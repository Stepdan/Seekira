#include "player.hpp"

#include <core/log/log.hpp>

namespace step::video::ff {

PlayerFF::PlayerFF() {}

PlayerFF::~PlayerFF() { stop(); }

void PlayerFF::open_file(const std::string& filename)
{
    m_parser = std::make_shared<ParserFF>();
    m_parser->open_file(filename);

    m_demuxer = std::make_shared<DemuxerQueue>(m_parser);
    m_stream_reader = std::make_shared<StreamReader>(m_demuxer);

    m_stream = m_stream_reader->get_best_video_stream();
}

void PlayerFF::play()
{
    run_worker();
    set_playing_state(PlayingState::Playing);
}

void PlayerFF::pause()
{
    stop_worker();
    set_playing_state(PlayingState::Paused);
}

void PlayerFF::stop()
{
    stop_worker();
    set_playing_state(PlayingState::Stopped);
}

void PlayerFF::set_media_status(MediaStatus status)
{
    m_media_status = status;
    m_player_observers.perform_for_each_event_handler(
        std::bind(&IPlayerEventObserver::on_media_status_changed, std::placeholders::_1, status));
}

void PlayerFF::set_playing_state(PlayingState state)
{
    m_playing_state = state;
    m_player_observers.perform_for_each_event_handler(
        std::bind(&IPlayerEventObserver::on_playing_state_changed, std::placeholders::_1, state));
}

void PlayerFF::run_worker() { ThreadWorker::run_worker(); }

void PlayerFF::stop_worker() { ThreadWorker::stop_worker(); }

void PlayerFF::worker_thread()
{
    while (!m_need_stop && !m_stream->is_eof_reached())
    {
        try
        {
            auto frame_ptr = m_stream->read_frame();
            if (frame_ptr)
            {
                m_frame_observers.perform_for_each_event_handler(
                    std::bind(&IFrameSourceObserver::process_frame, std::placeholders::_1, frame_ptr));
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

    if (!m_need_stop && m_exception_ptr)
    {
        set_playing_state(PlayingState::Error);
    }

    if (!m_need_stop && m_stream->is_eof_reached())
    {
        set_playing_state(PlayingState::Stopped);
    }
}

/* clang-format off */
void PlayerFF::register_observer(IFrameSourceObserver* observer) { m_frame_observers.register_event_handler(observer); }
void PlayerFF::register_observer(IPlayerEventObserver* observer) { m_player_observers.register_event_handler(observer); }
void PlayerFF::unregister_observer(IFrameSourceObserver* observer) { m_frame_observers.unregister_event_handler(observer); }
void PlayerFF::unregister_observer(IPlayerEventObserver* observer) { m_player_observers.unregister_event_handler(observer); }
/* clang-format on */

}  // namespace step::video::ff