#pragma once

#include <core/exception/assert.hpp>

#include <string>
#include <unordered_map>

namespace step {

class IConnectable
{
public:
    using ConnId = std::string;

public:
    IConnectable() = default;
    IConnectable(const ConnId& id) : m_conn_id(id) { STEP_ASSERT(!id.empty(), "Connectable id can't be empty!"); }
    virtual ~IConnectable() = default;

    void set_conn_id(const ConnId& id) { m_conn_id = id; }
    const ConnId& get_conn_id() { return m_conn_id; }

private:
    ConnId m_conn_id;
};

class IConnectionObject : public IConnectable
{
public:
    IConnectionObject() = default;
    IConnectionObject(const ConnId& id) : IConnectable(id) {}
    virtual ~IConnectionObject();
};

class IConnectionSource : public IConnectable
{
public:
    IConnectionSource() = default;
    IConnectionSource(const ConnId& id) : IConnectable(id) {}
    virtual ~IConnectionSource();

    virtual void connect(IConnectionObject*) = 0;
    virtual void disconnect(IConnectionObject*) = 0;
};

class Connector
{
public:
    static Connector& instance();

    static void register_conn_source(IConnectionSource* source);
    static void unregister_conn_source(IConnectionSource* source);

    static void connect(IConnectionObject* connectable_obj);
    static void disconnect(IConnectionObject* connectable_obj);

private:
    void register_conn_source_impl(IConnectionSource* source);
    void unregister_conn_source_impl(IConnectionSource* source);

    void connect_impl(IConnectionObject* connectable_obj);
    void disconnect_impl(IConnectionObject* connectable_obj);

private:
    Connector();
    ~Connector() = default;
    Connector(const Connector&) = delete;
    Connector(Connector&&) = delete;
    Connector& operator=(const Connector&) = delete;
    Connector& operator=(Connector&&) = delete;

private:
    std::unordered_map<IConnectable::ConnId, IConnectionSource*> m_connections;
};

}  // namespace step