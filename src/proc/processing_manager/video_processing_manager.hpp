#pragma once

#include <core/threading/thread_pool_execute_policy.hpp>

#include <proc/processor/video_processor.hpp>

namespace step::proc {

class VideoProcessingManager : public IVideoProcessorObserver, public video::IFrameSource
{
public:
    VideoProcessingManager(const ObjectPtrJSON& cfg);

public:
    IVideoProcessor* get_video_processor() const;

    // video::IFrameSource
public:
    void register_observer(video::IFrameSourceObserver* observer) override;
    void unregister_observer(video::IFrameSourceObserver* observer) override;

    // IVideoProcessorObserver
private:
    void on_finished(const VideoProcessorId& id, const VideoProcessorInfo& data) override;

private:
    std::unique_ptr<IVideoProcessor> m_processor;

    step::EventHandlerList<video::IFrameSourceObserver, threading::ThreadPoolExecutePolicy<0>> m_frame_observers;
};

}  // namespace step::proc