#pragma once

#include "ff_types.hpp"
#include "ff_decoder_video.hpp"

#include <core/base/interfaces/event_handler_list.hpp>
#include <core/threading/thread_worker.hpp>

#include <video/frame/interfaces/frame_interfaces.hpp>

#include <functional>
#include <map>

namespace step::video::ff {

class FFDemuxerInterruptCallback : public AVIOInterruptCB
{
public:
    using CallbackType = std::function<void()>;

public:
    FFDemuxerInterruptCallback(CallbackType callback) : m_callback(callback) {}

    static int handler(void* obj);

private:
    CallbackType m_callback;
};

class FFDemuxer : public threading::ThreadWorker, public IFrameSource
{
public:
    FFDemuxer();
    ~FFDemuxer();

    void load(const std::string& url_str);

private:
    bool find_streams();

    // threading::ThreadWorker
private:
    void worker_thread();

    // IFrameSource
public:
    void register_observer(IFrameSourceObserver* observer) override
    {
        m_frame_observers.register_event_handler(observer);
    }

    void unregister_observer(IFrameSourceObserver* observer) override
    {
        m_frame_observers.unregister_event_handler(observer);
    }

private:
    FormatContextInputSafe m_format_input_ctx;
    std::map<StreamId, AVStream*> m_video_streams;

    std::unique_ptr<FFDecoderVideo> m_video_decoder;

    step::EventHandlerList<IFrameSourceObserver> m_frame_observers;

    bool m_is_local_file{true};
};

}  // namespace step::video::ff