#pragma once

#include <video/ffmpeg/reader/reader.hpp>

#include <proc/interfaces/video_processor_interface.hpp>

namespace step::proc {

class VideoProcessingManager : public video::ff::ReaderFF
{
public:
    VideoProcessingManager(const ObjectPtrJSON& cfg);

public:
    void reader_process_frame(video::FramePtr frame) override;

private:
    void process_video_info(const VideoProcessorInfo& info);

private:
    VideoProcessorTaskPtr m_video_processing_task;
};

}  // namespace step::proc