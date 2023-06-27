#pragma once

#include <gui/utils/objects_connector.hpp>

#include <QObject>
#include <QString>

/// Макрос для добавления идентификаторов.
/// Автоматически генерирует объявление свойства и реализацию метода доступа, пригодные для использования в QML
#define ADD_PROPERTY(prop_name)                                                                                        \
    Q_PROPERTY(QString prop_name READ prop_name CONSTANT)                                                              \
public:                                                                                                                \
    static QString prop_name() { return #prop_name; }

namespace step::gui {

/// Набор уникальных идентификаторов для использования в ObjectsConnector, которые видно из QML
class ObjectsConnectorID : public QObject
{
    Q_OBJECT

    //Идентификаторы для GUI
    ADD_PROPERTY(SET_QML_IN_MAIN_WINDOW)

    // ObjectName главного QML окна
    ADD_PROPERTY(QML_MAIN_WINDOW)

    // ObjectName QML окна плеера
    ADD_PROPERTY(QML_PLAYER_WINDOW)

    ADD_PROPERTY(VIDEO_FRAME_UPDATED)

    ADD_PROPERTY(PLAYBACK_POS_UPDATED)

public:
    explicit ObjectsConnectorID(QObject* parent = nullptr) : QObject(parent) {}
};

}  // namespace step::gui