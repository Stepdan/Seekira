#pragma once

#include <proc/pipeline/async/async_pipeline.hpp>
#include <proc/pipeline/sync/sync_pipeline.hpp>
#include <video/frame/interfaces/frame_interfaces.hpp>

namespace step::proc {

using FrameSyncPipeline = SyncPipeline<video::Frame>;

class FrameAsyncPipeline : public AsyncPipeline<video::Frame>, public video::IFrameSourceObserver
{
public:
    FrameAsyncPipeline(const ObjectPtrJSON& config);

    // IFrameSourceObserver
public:
    void process_frame(video::FramePtr) override;
};

}  // namespace step::proc