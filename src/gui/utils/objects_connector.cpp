#include "objects_connector.hpp"

#include <cassert>

namespace step::gui::utils {

static const std::string TYPE_SIGNAL = "TYPE_SIGNAL";
static const std::string TYPE_SLOT = "TYPE_SLOT";

ObjectsConnector::MetaObjectsMap ObjectsConnector::m_meta_collector = ObjectsConnector::MetaObjectsMap();

ObjectsConnector::ObjectsConnector(QObject* parent /* = 0*/) : QObject(parent) {}

void ObjectsConnector::register_emitter(const QString& id, QObject* sender, const QString& signal,
                                        bool queued /* = false*/)
{
    const std::string signal_str = signal.startsWith('2') ? signal.toStdString() : QString("2" + signal).toStdString();

    for (const auto& record : m_meta_collector[id][TYPE_SLOT])
    {
        if (sender != record.first)
        {
            const auto result =
                connect(sender, signal_str.c_str(), record.first, record.second.c_str(),
                        queued ? static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection)
                               : Qt::UniqueConnection);
            assert(result && "bad emitter");
        }
    }

    register_meta_type(id, TYPE_SIGNAL, std::make_pair(sender, signal_str));
}

void ObjectsConnector::register_receiver(const QString& id, QObject* receiver, const QString& slot,
                                         bool queued /* = false*/)
{
    const std::string slot_str = slot.startsWith('1') ? slot.toStdString() : QString("1" + slot).toStdString();

    for (const auto& record : m_meta_collector[id][TYPE_SIGNAL])
    {
        if (receiver != record.first)
        {
            const auto result =
                connect(record.first, record.second.c_str(), receiver, slot_str.c_str(),
                        queued ? static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection)
                               : Qt::UniqueConnection);
            assert(result && "bad receiver");
        }
    }

    register_meta_type(id, TYPE_SLOT, std::make_pair(receiver, slot_str));
}

void ObjectsConnector::unregister_receiver(const QString& id, QObject* receiver, const QString& slot)
{
    const std::string slot_str = slot.startsWith('1') ? slot.toStdString() : QString("1" + slot).toStdString();

    for (const auto& record : m_meta_collector[id][TYPE_SIGNAL])
    {
        if (receiver != record.first)
            disconnect(record.first, record.second.c_str(), receiver, slot_str.c_str());
    }

    m_meta_collector[id][TYPE_SLOT].remove(std::make_pair(receiver, slot_str));
}

void ObjectsConnector::unregister_emitter(const QString& id, QObject* sender, const QString& signal)
{
    const std::string signal_str = signal.startsWith('2') ? signal.toStdString() : QString("2" + signal).toStdString();

    for (const auto& record : m_meta_collector[id][TYPE_SLOT])
    {
        if (sender != record.first)
            disconnect(sender, signal_str.c_str(), record.first, record.second.c_str());
    }
    m_meta_collector[id][TYPE_SIGNAL].remove(std::make_pair(sender, signal_str));
}

void ObjectsConnector::register_meta_type(const QString& id, const std::string& meta_type,
                                          const std::pair<const QObject*, const std::string>& meta_pair)
{
    m_meta_collector[id][meta_type].insert(m_meta_collector[id][meta_type].end(), meta_pair);

    connect(meta_pair.first, &QObject::destroyed, [=]() { m_meta_collector[id][meta_type].remove(meta_pair); });
}

}  // namespace step::gui::utils