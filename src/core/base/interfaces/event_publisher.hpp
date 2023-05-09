#pragma once

namespace step {

template <typename IHandler>
class EventPublisher
{
protected:
    using Handler = IHandler;

public:
    virtual ~EventPublisher() = default;

    virtual void register_event_handler(Handler*) = 0;
    virtual void unregister_event_handler(Handler*) = 0;
};

}  // namespace step