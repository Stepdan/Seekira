#pragma once

#include <map>
#include <list>
#include <string>
#include <utility>

#include <QObject>
#include <QString>

namespace step::gui::utils {

class ObjectsConnector : public QObject
{
    Q_OBJECT

public:
    explicit ObjectsConnector(QObject* parent = 0);

private:
    ObjectsConnector(const ObjectsConnector&);
    ObjectsConnector& operator=(const ObjectsConnector&);

public:
    Q_INVOKABLE static void register_emitter(const QString& id, QObject* sender, const QString& signal,
                                             bool queued = false);
    Q_INVOKABLE static void register_receiver(const QString& id, QObject* receiver, const QString& slot,
                                              bool queued = false);
    Q_INVOKABLE static void unregister_receiver(const QString& id, QObject* receiver, const QString& slot);
    Q_INVOKABLE static void unregister_emitter(const QString& id, QObject* sender, const QString& signal);

private:
    Q_INVOKABLE static void register_meta_type(const QString& id, const std::string& meta_type,
                                               const std::pair<const QObject*, const std::string>& meta_pair);

private:
    using MetaObjectsMap =
        std::map<QString, std::map<std::string, std::list<std::pair<const QObject*, const std::string>>>>;
    static MetaObjectsMap m_meta_collector;
};

}  // namespace step::gui::utils