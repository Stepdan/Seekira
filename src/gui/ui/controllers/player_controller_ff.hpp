#pragma once

#include <video/frame/interfaces/frame_interfaces.hpp>

#include <proc/video/video_processing_manager.hpp>

#include <gui/interfaces/player_controller.hpp>
#include <gui/interfaces/video_frame_provider.hpp>

#include <QObject>

#include <mutex>

namespace step::gui {

class PlayerControllerFF : public IPlayerController, public video::ff::IReaderEventObserver
{
    Q_OBJECT

public:
    enum class PlayerMode
    {
        Undefined,
        Player,         // Работа в режиме плеера, используем Qt плеер
        ReadByRequest,  // Работа в режиме чтения по запросуб используем ReaderFF
        ReadContinuously,  // Работа в режиме непрерывного чтения, используем ReaderFF
    };

public:
    PlayerControllerFF(QObject* parent = nullptr);
    ~PlayerControllerFF();

    // IPlayerController
public:
    IVideoFrameProvider* get_video_frame_provider() const override;

    bool open_file(const QString& filename) override;

    QMediaPlayer::State get_state() const override;

    void play_state_switch() override;
    void step_rewind(Enums::PlayerDirection) override;
    void step_frame(Enums::PlayerDirection) override;

    void reset() override;

    void playback_set_position_slot(double value) override;
    qint64 get_position() const override;
    qint64 get_duration() const override;

    // video::ff::IReaderEventObserver
public:
    void on_reader_state_changed(video::ff::ReaderState state);

signals:
    void update_playback_pos_signal();

private:
    std::unique_ptr<IVideoFrameProvider> m_video_frame_provider;

    std::unique_ptr<proc::VideoProcessingManager> m_video_proc_manager;

    QMediaPlayer::State m_state{QMediaPlayer::State::StoppedState};

    bool m_is_loaded{false};

    mutable std::mutex m_guard;
};

}  // namespace step::gui