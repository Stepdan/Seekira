#pragma once

#include "video_frame_provider.hpp"
#include "declare_metatype.hpp"

#include <QMediaPlayer>
#include <QString>

namespace step::gui {

class IPlayerController : public QObject
{
    Q_OBJECT

public:
    IPlayerController(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IPlayerController() = default;

    Q_INVOKABLE virtual IVideoFrameProvider* get_video_frame_provider() const = 0;

    Q_INVOKABLE virtual bool open_file(const QString& filename) = 0;

    Q_INVOKABLE virtual QMediaPlayer::State get_state() const = 0;

    Q_INVOKABLE virtual void play_state_switch() = 0;
    Q_INVOKABLE virtual void step_rewind(step::gui::Enums::PlayerDirection) = 0;
    Q_INVOKABLE virtual void step_frame(step::gui::Enums::PlayerDirection) = 0;

    Q_INVOKABLE virtual void reset() = 0;
};

}  // namespace step::gui