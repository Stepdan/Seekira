#pragma once

#include <QObject>

namespace step::gui {

class Enums : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    Q_ENUMS(PlayerDirection)

public:
    enum PlayerDirection
    {
        PLAYER_DIRECTION_FORWARD,
        PLAYER_DIRECTION_BACKWARD,
    };
};

}  // namespace step::gui

namespace step::gui {

void register_qml_enums();

}