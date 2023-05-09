#pragma once

#include <video/frame/interfaces/frame_interfaces.hpp>
#include <video/camera/interfaces/types/camera_settings.hpp>

#include <core/base/interfaces/event_handler_list.hpp>
#include <core/threading/thread_pool_execute_policy.hpp>

#include <atomic>

namespace step::video {

enum class CameraErrorStatus
{
    Unknown = 0,
    ReconnectFailed,
};

using CameraErrorCallback = std::function<void(CameraErrorStatus)>;

class ICameraSingleshot
{
public:
    virtual ~ICameraSingleshot() = default;

    // TODO: do NOT use infinite time. any implementation of this camera API has
    // to do the following:
    //  ```
    //      auto now = clock::now(); // use steady clock!
    //      auto max = std::chrono::time_point::max();
    //      assert(timeout <= max && now <= max - timeout);
    //  ```
    virtual Frame get_frame(Milliseconds timeout = Milliseconds(INFINITE_TIME)) = 0;
};

class ICameraStreaming : public IFrameSource
{
public:
    virtual ~ICameraStreaming() = default;
    virtual void start() = 0;
    virtual void stop() = 0;

public:
    void set_error_callback(CameraErrorCallback callback) { m_error_callback = callback; }

    bool is_streaming() const { return m_is_streaming; }

public:
    void register_observer(IFrameSourceObserver* observer) override
    {
        m_frame_observers.register_event_handler(observer);
    }

    void unregister_observer(IFrameSourceObserver* observer) override
    {
        m_frame_observers.unregister_event_handler(observer);
    }

protected:
    void handle_frame(Frame&& data)
    {
        auto frame_ptr = std::make_shared<Frame>(std::move(data));
        m_frame_observers.perform_for_each_event_handler(
            std::bind(&IFrameSourceObserver::process_frame, std::placeholders::_1, frame_ptr));
    }

protected:
    std::atomic_bool m_is_streaming{false};
    CameraErrorCallback m_error_callback;

    step::EventHandlerList<IFrameSourceObserver, threading::ThreadPoolExecutePolicy<0>> m_frame_observers;
};

}  // namespace step::video
