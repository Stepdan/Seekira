#pragma once

#include <core/pipeline/pipeline.hpp>
#include <video/frame/interfaces/frame_interfaces.hpp>

namespace step::proc {

class FramePipeline : public pipeline::Pipeline<video::Frame>, public video::IFrameSourceObserver
{
public:
    FramePipeline(const ObjectPtrJSON& config);

    // IFrameSourceObserver
private:
    void process_frame(video::FramePtr) override;
};

}  // namespace step::proc