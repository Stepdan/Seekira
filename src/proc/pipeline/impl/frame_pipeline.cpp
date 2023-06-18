#include "frame_pipeline.hpp"

#include <core/log/log.hpp>

namespace step::proc {

std::unique_ptr<FrameSyncPipeline> FrameSyncPipeline::create(const ObjectPtrJSON& pipeline_json)
{
    auto pipeline_ptr = std::make_unique<FrameSyncPipeline>();
    pipeline_ptr->initialize(pipeline_json);
    return pipeline_ptr;
}

std::unique_ptr<FrameAsyncPipeline> FrameAsyncPipeline::create(const ObjectPtrJSON& pipeline_json)
{
    auto pipeline_ptr = std::make_unique<FrameAsyncPipeline>();
    pipeline_ptr->initialize(pipeline_json);
    return pipeline_ptr;
}

void FrameAsyncPipeline::process_frame(video::FramePtr frame_ptr)
{
    auto frame = *frame_ptr;
    auto pipeline_data = proc::PipelineData<video::Frame>::create(std::move(frame));

    add_process_data(std::move(pipeline_data));
}

}  // namespace step::proc