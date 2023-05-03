#include "frame_pipeline.hpp"

#include <log/log.hpp>

namespace step::video {

FramePipeline::FramePipeline(const ObjectPtrJSON& config) : pipeline::Pipeline<Frame>(config) {}

void FramePipeline::process_frame(FramePtr frame_ptr)
{
    auto frame = *frame_ptr;
    auto pipeline_data = pipeline::PipelineData<Frame>::create(std::move(frame));

    add_process_data(pipeline_data);
}

}  // namespace step::video