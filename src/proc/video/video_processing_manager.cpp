#include "video_processing_manager.hpp"

#include <core/base/types/config_fields.hpp>
#include <core/task/settings_factory.hpp>
#include <core/task/task_factory.hpp>

namespace step::proc {

VideoProcessingManager::VideoProcessingManager(const ObjectPtrJSON& cfg)
    : m_video_processing_task(IVideoProcessorTask::from_abstract(
          CREATE_TASK_UNIQUE(CREATE_SETTINGS(json::get_object(cfg, CFG_FLD::VIDEO_PROCESSOR)))))
{
}

void VideoProcessingManager::reader_process_frame(video::FramePtr frame)
{
    auto video_info = m_video_processing_task->process(frame);
    *frame = video_info.data;
    process_video_info(video_info);
}

void VideoProcessingManager::process_video_info(const VideoProcessorInfo& info) {}

}  // namespace step::proc