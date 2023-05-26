#pragma once

#include <gui/utils/objects_connector.hpp>

#include <QObject>
#include <QString>

/// Макрос для добавления идентификаторов.
/// Автоматически генерирует объявление свойства и реализацию метода доступа, пригодные для использования в QML
#define ADD_PROPERTY(propName)                                                                                         \
    Q_PROPERTY(QString propName READ propName CONSTANT)                                                                \
public:                                                                                                                \
    static QString propName() { return #propName; }

namespace step::gui {

/// Набор уникальных идентификаторов для использования в ObjectsConnector, которые видно из QML
class ObjectsConnectorID : public QObject
{
    Q_OBJECT

    // ObjectName главного QML окна
    ADD_PROPERTY(QML_MAIN_WINDOW)

public:
    explicit ObjectsConnectorID(QObject* parent = nullptr) : QObject(parent) {}
};

}  // namespace step::gui