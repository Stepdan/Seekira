#include "frame_pipeline.hpp"

#include <core/log/log.hpp>

namespace step::proc {

FramePipeline::FramePipeline(const ObjectPtrJSON& config) : pipeline::Pipeline<video::Frame>(config) {}

void FramePipeline::process_frame(video::FramePtr frame_ptr)
{
    auto frame = *frame_ptr;
    auto pipeline_data = pipeline::PipelineData<video::Frame>::create(std::move(frame));

    add_process_data(std::move(pipeline_data));
}

}  // namespace step::proc