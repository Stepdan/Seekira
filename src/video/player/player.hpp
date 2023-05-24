#pragma once

#include "player_event_handler.hpp"

#include <core/threading/thread_worker.hpp>

#include <video/frame/interfaces/frame_interfaces.hpp>

#include <video/ffmpeg/decoding/stream_reader.hpp>
#include <video/ffmpeg/decoding/decoder_video.hpp>

namespace step::video::ff {

class PlayerFF : public step::threading::ThreadWorker, public IPlayerEventSource, public IFrameSource
{
public:
    PlayerFF();
    ~PlayerFF();

    void open_file(const std::string& filename);

    void play();
    void pause();
    void stop();
    void seek(TimestampFF pos);

    MediaStatus get_media_status() const;
    PlayingState get_playing_state() const;
    void set_media_status(MediaStatus);
    void set_playing_state(PlayingState);

public:
    void register_observer(IFrameSourceObserver* observer) override;
    void register_observer(IPlayerEventObserver* observer) override;

    void unregister_observer(IFrameSourceObserver* observer) override;
    void unregister_observer(IPlayerEventObserver* observer) override;

private:
    void run_worker() override;
    void stop_worker() override;
    void worker_thread() override;

private:
    std::shared_ptr<ParserFF> m_parser{nullptr};
    std::shared_ptr<IDemuxer> m_demuxer{nullptr};
    std::shared_ptr<IStreamReader> m_stream_reader{nullptr};

    StreamPtr m_stream{nullptr};

    MediaStatus m_media_status;
    PlayingState m_playing_state;

    step::EventHandlerList<IFrameSourceObserver, threading::ThreadPoolExecutePolicy<0>> m_frame_observers;
    step::EventHandlerList<IPlayerEventObserver, threading::ThreadPoolExecutePolicy<0>> m_player_observers;
};

}  // namespace step::video::ff