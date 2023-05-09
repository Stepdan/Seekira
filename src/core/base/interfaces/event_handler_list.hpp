#pragma once

#include "event_publisher.hpp"

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
    virtual void register_event_handler(EventHandler* handler) override
    {
        if (!handler)
            return;

        std::unique_lock<std::recursive_mutex> lock(m_access_guard);

        if (std::ranges::none_of(m_subscribers, [&handler](const auto& item) { return handler == item; }))
            m_subscribers.push_back(handler);
    }

    virtual void unregister_event_handler(EventHandler* handler) override
    {
        if (!handler)
            return;

        std::unique_lock<std::recursive_mutex> lock(m_access_guard);

        if (auto it = std::find(m_subscribers.begin(), m_subscribers.end(), handler); it != m_subscribers.end())
            m_subscribers.erase(it);
    }

    template <typename FunctorType>
    void perform_for_each_event_handler(FunctorType functor)
    {
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
    std::vector<EventHandler*> m_subscribers;
    std::recursive_mutex m_access_guard;
    std::recursive_mutex m_notification_guard;
    DeliveryPolicy m_exec_policy;
};

}  // namespace step