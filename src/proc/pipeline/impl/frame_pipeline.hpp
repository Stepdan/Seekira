#pragma once

#include <proc/pipeline/async/async_pipeline.hpp>
#include <proc/pipeline/sync/sync_pipeline.hpp>
#include <video/frame/interfaces/frame_interfaces.hpp>

namespace step::proc {

class FrameSyncPipeline : public SyncPipeline<video::Frame>
{
public:
    static std::unique_ptr<FrameSyncPipeline> create(const ObjectPtrJSON& pipeline_json);
};

class FrameAsyncPipeline : public AsyncPipeline<video::Frame>, public video::IFrameSourceObserver
{
public:
    static std::unique_ptr<FrameAsyncPipeline> create(const ObjectPtrJSON& pipeline_json);
    // IFrameSourceObserver
public:
    void process_frame(video::FramePtr) override;
};

}  // namespace step::proc