#pragma once

#include "ff_stream_info.hpp"

#include <core/base/interfaces/event_handler_list.hpp>
#include <core/threading/thread_pool_execute_policy.hpp>
#include <core/threading/thread_worker.hpp>

#include <video/frame/interfaces/frame_interfaces.hpp>

#include <memory>

namespace step::video::ff {

enum class FFStreamError
{
    Undefined,
    None,
    InvalidFormatContext,
    InvalidOpen,
    InvalidInfo,
    InvalidBestStream,
    InvalidCodecContext,
    InvalidCodec,
    InvalidFrame,
    InvalidPacket,
    InvalidSwsContext,
};

class FFStream : public threading::ThreadWorker, public IFrameSource
{
public:
    FFStream();
    ~FFStream();

    FFStreamInfo get_stream_info() const;

    FFStreamError open(const std::string& filename, PixFmt pix_fmt = PixFmt::BGR, FrameSize frame_size = FrameSize());
    FFStreamError close();

    void seek(Timestamp ts);

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

    // ThreadWorker
private:
    void worker_thread() override;

private:
    FramePtr read_frame();
    void seek_frame();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    EventHandlerList<IFrameSourceObserver, threading::ThreadPoolExecutePolicy<0>> m_frame_observers;
};

}  // namespace step::video::ff