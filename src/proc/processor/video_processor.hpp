#pragma once

#include <proc/interfaces/video_processor_interface.hpp>

namespace step::proc {

class VideoProcessor : public IVideoProcessor, public ISerializable
{
public:
    static std::unique_ptr<IVideoProcessor> create(const ObjectPtrJSON& config);

public:
    VideoProcessor(const ObjectPtrJSON& config);
    ~VideoProcessor();

private:
    VideoProcessorInfo thread_pool_worker_process_data(const video::FramePtr& data) override;
    void process_frame(video::FramePtr frame) override;

    void deserialize(const ObjectPtrJSON& container) override;

private:
    VideoProcessorTaskPtr m_task;
};

}  // namespace step::proc