#include "frame_pipeline.hpp"

#include <core/log/log.hpp>

namespace step::proc {

FrameAsyncPipeline::FrameAsyncPipeline(const ObjectPtrJSON& config) : proc::AsyncPipeline<video::Frame>(config) {}

void FrameAsyncPipeline::process_frame(video::FramePtr frame_ptr)
{
    auto frame = *frame_ptr;
    auto pipeline_data = proc::PipelineData<video::Frame>::create(std::move(frame));

    add_process_data(std::move(pipeline_data));
}

}  // namespace step::proc