#include "connector.hpp"

#include <core/exception/assert.hpp>

namespace step {

IConnectionObject::~IConnectionObject() { Connector::disconnect(this); }

IConnectionSource::~IConnectionSource() { Connector::unregister_conn_source(this); }

Connector& Connector::instance()
{
    static Connector obj;
    return obj;
}

Connector::Connector() = default;

void Connector::register_conn_source(IConnectionSource* source)
{
    Connector::instance().register_conn_source_impl(source);
}

void Connector::unregister_conn_source(IConnectionSource* source)
{
    Connector::instance().unregister_conn_source_impl(source);
}

void Connector::connect(IConnectionObject* connectable_obj) { Connector::instance().connect_impl(connectable_obj); }

void Connector::disconnect(IConnectionObject* connectable_obj)
{
    Connector::instance().disconnect_impl(connectable_obj);
}

void Connector::register_conn_source_impl(IConnectionSource* source)
{
    STEP_ASSERT(source, "Can't register empty source!");
    STEP_ASSERT(m_connections.find(source->get_conn_id()) == m_connections.cend(),
                "Can't register source {}: already exists!", source->get_conn_id());

    m_connections[source->get_conn_id()] = source;
}

void Connector::unregister_conn_source_impl(IConnectionSource* source)
{
    STEP_ASSERT(source, "Can't unregister empty source!");
    auto it = m_connections.find(source->get_conn_id());
    STEP_ASSERT(it != m_connections.cend(), "Can't unregister source {}: doesn't exist!", source->get_conn_id());

    m_connections.erase(source->get_conn_id());
}

void Connector::connect_impl(IConnectionObject* connectable_obj)
{
    STEP_ASSERT(connectable_obj, "Can't connect empty connectable!");
    auto it = m_connections.find(connectable_obj->get_conn_id());
    STEP_ASSERT(it != m_connections.cend(), "Can't connect obj: source {} doesn't exists!",
                connectable_obj->get_conn_id());
    STEP_ASSERT(it->second, "Can't connect obj: source {} exists but empty!", connectable_obj->get_conn_id());

    it->second->connect(connectable_obj);
}

void Connector::disconnect_impl(IConnectionObject* connectable_obj)
{
    STEP_ASSERT(connectable_obj, "Can't disconnect empty connectable!");
    auto it = m_connections.find(connectable_obj->get_conn_id());
    STEP_ASSERT(it != m_connections.cend(), "Can't disconnect obj: source {} doesn't exists!",
                connectable_obj->get_conn_id());
    STEP_ASSERT(it->second, "Can't disconnect obj: source {} exists but empty!", connectable_obj->get_conn_id());

    it->second->disconnect(connectable_obj);
}

}  // namespace step