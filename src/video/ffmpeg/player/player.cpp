#include "player.hpp"

namespace step::video::ff {

PlayerFF::PlayerFF() = default;
PlayerFF::~PlayerFF() = default;

void PlayerFF::open_file(const std::string& filename)
{
    m_parser = std::make_shared<ParserFF>();
    m_parser->open_file(filename);

    m_demuxer = std::make_shared<DemuxerQueue>(m_parser);
    m_stream_reader = std::make_shared<StreamReader>(m_demuxer);
}

void PlayerFF::play()
{
    run_worker();
    m_playing_state = PlayingState::Playing;
}

void PlayerFF::pause()
{
    stop_worker();
    m_playing_state = PlayingState::Paused;
}

void PlayerFF::stop()
{
    stop_worker();
    m_playing_state = PlayingState::Stopped;
}

void PlayerFF::run_worker() { ThreadWorker::run_worker(); }

void PlayerFF::stop_worker() { ThreadWorker::stop_worker(); }

void PlayerFF::worker_thread()
{
    while (!m_need_stop)
    {
    }
}

/* clang-format off */
void PlayerFF::register_observer(IFrameSourceObserver* observer) { m_frame_observers.register_event_handler(observer); }
void PlayerFF::register_observer(IPlayerEventObserver* observer) { m_player_observers.register_event_handler(observer); }
void PlayerFF::unregister_observer(IFrameSourceObserver* observer) { m_frame_observers.unregister_event_handler(observer); }
void PlayerFF::unregister_observer(IPlayerEventObserver* observer) { m_player_observers.unregister_event_handler(observer); }
/* clang-format on */

}  // namespace step::video::ff