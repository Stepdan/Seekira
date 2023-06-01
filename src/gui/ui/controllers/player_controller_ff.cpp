#include "player_controller_ff.hpp"

#include <core/log/log.hpp>
#include <core/exception/assert.hpp>

#include <gui/utils/log_handler.hpp>

#include <gui/ui/video/video_frame_provider_ff.hpp>

namespace {
constexpr step::video::ff::TimestampFF STEP_VALUE = 10 * step::video::ff::AV_SECOND;
}

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
    m_video_reader.reset();
    m_state = QMediaPlayer::State::StoppedState;
    m_is_loaded = false;
}

bool PlayerControllerFF::open_file(const QString& filename)
{
    {
        std::scoped_lock(m_guard);
        STEP_LOG(L_DEBUG, "PlayerControllerFF: open file {}", filename);

        reset();

        m_video_reader = std::make_unique<video::ff::ReaderFF>();

        if (!m_video_reader->open_file(filename.toStdString()))
        {
            STEP_LOG(L_ERROR, "Can't open file {}", filename);
            return false;
        }

        auto* frame_observer = dynamic_cast<step::gui::VideoFrameProviderFF*>(m_video_frame_provider.get());
        STEP_ASSERT(frame_observer, "Invalid cast to VideoFrameProviderFF*");

        m_video_reader->register_observer(frame_observer);
        m_video_reader->register_observer(this);

        // Открыли файл - ставим на паузу
        m_state = QMediaPlayer::State::PausedState;

        m_is_loaded = true;
    }

    // Прочитаем первый кадр, чтобы показать на экране и встать в 0
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
            m_video_reader->stop();
            m_video_reader->start(video::ff::ReadingMode::Continuously);
            break;
        }
        case QMediaPlayer::State::PausedState: {
            m_video_reader->stop();
            break;
        }
        case QMediaPlayer::State::StoppedState: {
            m_video_reader->stop();
            m_video_reader->set_position(0);
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

    const auto sign = (direction == Enums::PLAYER_DIRECTION_BACKWARD) ? -1 : 1;
    m_video_reader->set_position(m_video_reader->get_position() + sign * STEP_VALUE);
}

void PlayerControllerFF::step_frame(Enums::PlayerDirection direction)
{
    std::scoped_lock(m_guard);

    if (m_state == QMediaPlayer::State::StoppedState)
        return;

    // принудительно останавливаем чтение и настраиваем в режим чтения по запросу
    m_video_reader->stop();
    m_state = QMediaPlayer::State::PausedState;

    m_video_reader->start(video::ff::ReadingMode::ByRequest);

    if (direction == Enums::PLAYER_DIRECTION_FORWARD)
        m_video_reader->request_read();
    else
        m_video_reader->request_read_prev();
}

void PlayerControllerFF::on_reader_state_changed(video::ff::ReaderState state)
{
    std::scoped_lock(m_guard);

    if (state == video::ff::ReaderState::Error)
    {
        STEP_LOG(L_ERROR, "ReaderFF error handled!");
        reset();
    }
}

}  // namespace step::gui