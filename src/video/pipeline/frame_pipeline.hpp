#pragma once

#include <core/pipeline/pipeline.hpp>
#include <video/frame/interfaces/frame_interfaces.hpp>

namespace step::video {

class FramePipeline : public pipeline::Pipeline<Frame>, public video::IFrameSourceObserver
{
public:
    FramePipeline(const ObjectPtrJSON& config);

    // IFrameSourceObserver
private:
    void process_frame(FramePtr) override;
};

}  // namespace step::video