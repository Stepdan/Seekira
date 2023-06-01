#pragma once

#include "event_publisher.hpp"

#include <core/log/log.hpp>

#include <algorithm>
#include <functional>
#include <mutex>
#include <vector>

namespace step {

class SyncExecutePolicy
{
public:
    SyncExecutePolicy() = default;
    ~SyncExecutePolicy() = default;
    SyncExecutePolicy(const SyncExecutePolicy&) = delete;
    SyncExecutePolicy& operator=(const SyncExecutePolicy&) = delete;

    void operator()(std::function<void()> functor) { functor(); }
};

template <typename IHandler, class DeliveryPolicy = SyncExecutePolicy>
class EventHandlerList : virtual public EventPublisher<IHandler>
{
protected:
    using EventHandler = typename EventPublisher<IHandler>::Handler;

public:
    void disable()
    {
        if (!m_enabled)
            return;

        std::unique_lock<std::recursive_mutex> lock(m_access_guard);
        m_cache.clear();
        swap_cache();
        m_enabled = false;
    }

    void enable()
    {
        if (m_enabled)
            return;

        std::unique_lock<std::recursive_mutex> lock(m_access_guard);
        swap_cache();
        m_enabled = true;
    }

public:
    virtual void register_event_handler(EventHandler* handler) override
    {
        if (!handler)
            return;

        if (!m_enabled)
        {
            STEP_LOG(L_WARN, "Can't register event handler: disabled!");
            return;
        }

        std::unique_lock<std::recursive_mutex> lock(m_access_guard);

        if (std::ranges::none_of(m_subscribers, [&handler](const auto& item) { return handler == item; }))
            m_subscribers.push_back(handler);
    }

    virtual void unregister_event_handler(EventHandler* handler) override
    {
        if (!handler)
            return;

        if (!m_enabled)
        {
            STEP_LOG(L_WARN, "Can't unregister event handler: disabled!");
            return;
        }

        std::unique_lock<std::recursive_mutex> lock(m_access_guard);

        if (auto it = std::find(m_subscribers.begin(), m_subscribers.end(), handler); it != m_subscribers.end())
            m_subscribers.erase(it);
    }

    template <typename FunctorType>
    void perform_for_each_event_handler(FunctorType functor)
    {
        if (!m_enabled)
            STEP_LOG(L_WARN, "EventHandlerList is disabled!");

        std::vector<EventHandler*> subscribersCopy;

        {
            std::unique_lock<std::recursive_mutex> lock(m_access_guard);
            subscribersCopy = m_subscribers;
        }

        {
            std::unique_lock<std::recursive_mutex> lock(m_notification_guard);

            for (const auto& handler : subscribersCopy)
            {
                m_exec_policy(std::bind(functor, handler));
            }
        }
    }

    template <typename F, typename... ARGS>
    void perform_all(F function, ARGS&&... args)
    {
        PerformForEachEventHandler(std::bind(function, std::placeholders::_1, std::forward<ARGS>(args)...));
    }

private:
    void swap_cache() { m_subscribers.swap(m_cache); }

private:
    std::vector<EventHandler*> m_subscribers;
    std::recursive_mutex m_access_guard;
    std::recursive_mutex m_notification_guard;
    DeliveryPolicy m_exec_policy;

    std::vector<EventHandler*> m_cache;
    bool m_enabled{true};
};

}  // namespace step