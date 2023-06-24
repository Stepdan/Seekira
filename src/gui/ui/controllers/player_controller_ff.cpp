#include "player_controller_ff.hpp"

#include <core/log/log.hpp>
#include <core/exception/assert.hpp>
#include <core/base/types/config_fields.hpp>
#include <core/base/json/json_utils.hpp>

#include <gui/utils/log_handler.hpp>

#include <gui/ui/video/video_frame_provider_ff.hpp>

#include <QUrl>

namespace {
constexpr step::video::ff::TimestampFF STEP_VALUE = 2 * step::video::ff::AV_SECOND;

const std::string VIDEO_PROCESSING_CONFIG_PATH =
    "C:/Work/StepTech/StepKit/src/gui/resources/video_processing/video_processing_manager.json";
}  // namespace

namespace step::gui {

PlayerControllerFF::PlayerControllerFF(QObject* parent /*= nullptr*/)
    : m_video_frame_provider(std::make_unique<VideoFrameProviderFF>(this)), m_state(QMediaPlayer::State::StoppedState)
{
}

PlayerControllerFF::~PlayerControllerFF() { reset(); }

IVideoFrameProvider* PlayerControllerFF::get_video_frame_provider() const
{
    STEP_ASSERT(m_video_frame_provider, "m_video_frame_provider is invalid!");
    return m_video_frame_provider.get();
}

void PlayerControllerFF::reset()
{
    m_video_proc_manager.reset();
    m_state = QMediaPlayer::State::StoppedState;
    m_is_loaded = false;
}

bool PlayerControllerFF::open_file(const QString& filename)
{
    {
        std::scoped_lock(m_guard);

        reset();

        auto cfg = json::utils::from_file(VIDEO_PROCESSING_CONFIG_PATH);
        m_video_proc_manager = std::make_unique<proc::VideoProcessingManager>(cfg);

        auto filename_opt = json::get_opt<std::string>(cfg, CFG_FLD::FILENAME);
        STEP_ASSERT(filename_opt.has_value(), "No field {} in config", CFG_FLD::FILENAME);
        STEP_LOG(L_DEBUG, "PlayerControllerFF: open file {}", filename_opt.value());
        if (!m_video_proc_manager->open_file(filename_opt.value()))
        {
            STEP_LOG(L_ERROR, "Can't open file {}", filename);
            return false;
        }

        auto* frame_observer = dynamic_cast<step::gui::VideoFrameProviderFF*>(m_video_frame_provider.get());
        STEP_ASSERT(frame_observer, "Invalid cast to VideoFrameProviderFF*");

        m_video_proc_manager->register_observer(frame_observer);
        m_video_proc_manager->register_observer(this);

        // Открыли файл - ставим на паузу
        m_state = QMediaPlayer::State::PausedState;

        m_is_loaded = true;
    }

    // Прочитаем первый кадр, чтобы показать на экране и встать в 0
    m_video_proc_manager->start();
    step_frame(Enums::PLAYER_DIRECTION_FORWARD);

    return true;
}

QMediaPlayer::State PlayerControllerFF::get_state() const
{
    std::scoped_lock(m_guard);
    return m_state;
}

void PlayerControllerFF::play_state_switch()
{
    std::scoped_lock(m_guard);
    // StoppedState означает, что мы либо не открыли файл, либо "тотально" закончили с файлом
    if (m_state == QMediaPlayer::State::StoppedState)
        return;

    // Задаем значение-антагонист текущего состояния (play/pause)
    const auto state = (m_state == QMediaPlayer::State::PlayingState) ? QMediaPlayer::State::PausedState
                                                                      : QMediaPlayer::State::PlayingState;
    switch (state)
    {
        case QMediaPlayer::State::PlayingState: {
            STEP_LOG(L_DEBUG, "PLAY");
            m_video_proc_manager->play();
            break;
        }
        case QMediaPlayer::State::PausedState: {
            STEP_LOG(L_DEBUG, "PAUSE");
            m_video_proc_manager->pause();
            break;
        }
        case QMediaPlayer::State::StoppedState: {
            STEP_LOG(L_DEBUG, "STOP");
            m_video_proc_manager->stop();
            break;
        }
    }

    m_state = state;
}

void PlayerControllerFF::step_rewind(Enums::PlayerDirection direction)
{
    std::scoped_lock(m_guard);

    if (m_state == QMediaPlayer::State::StoppedState)
        return;

    // Не меняем m_state, так как если мы на паузе, то просто сделаем seek
    // Иначе делаем seek и проигрываем дальше
    direction == Enums::PlayerDirection::PLAYER_DIRECTION_FORWARD ? m_video_proc_manager->rewind_forward()
                                                                  : m_video_proc_manager->rewind_backward();
}

void PlayerControllerFF::step_frame(Enums::PlayerDirection direction)
{
    std::scoped_lock(m_guard);

    if (m_state == QMediaPlayer::State::StoppedState)
        return;

    // принудительно останавливаем чтение
    //m_video_proc_manager->pause();
    //m_state = QMediaPlayer::State::PausedState;

    direction == Enums::PlayerDirection::PLAYER_DIRECTION_FORWARD ? m_video_proc_manager->step_forward()
                                                                  : m_video_proc_manager->step_backward();
}

void PlayerControllerFF::on_reader_state_changed(video::ff::ReaderState state)
{
    std::scoped_lock(m_guard);

    if (state == video::ff::ReaderState::Error)
    {
        STEP_LOG(L_ERROR, "ReaderFF error handled!");
        reset();
    }

    if (state == video::ff::ReaderState::Reading)
        QMediaPlayer::State::PlayingState;

    if (state == video::ff::ReaderState::Paused)
        QMediaPlayer::State::PausedState;

    if (state == video::ff::ReaderState::Stopped)
        QMediaPlayer::State::StoppedState;
}

}  // namespace step::gui