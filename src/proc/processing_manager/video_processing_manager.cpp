#include "video_processing_manager.hpp"

#include <core/base/types/config_fields.hpp>

namespace step::proc {

VideoProcessingManager::VideoProcessingManager(const ObjectPtrJSON& cfg)
    : m_processor(VideoProcessor::create(json::get_object(cfg, CFG_FLD::VIDEO_PROCESSOR)))
{
}

IVideoProcessor* VideoProcessingManager::get_video_processor() const
{
    STEP_ASSERT(m_processor, "Invalid video processor");
    return m_processor.get();
}

void VideoProcessingManager::on_finished(const VideoProcessorId& id, const VideoProcessorInfo& data)
{
    // Здесь обрабатываем поступившую информацию

    auto frame_ptr = std::make_shared<video::Frame>(std::move(data.data));
    m_frame_observers.perform_for_each_event_handler(
        std::bind(&video::IFrameSourceObserver::process_frame, std::placeholders::_1, frame_ptr));
}

void VideoProcessingManager::register_observer(video::IFrameSourceObserver* observer)
{
    m_frame_observers.register_event_handler(observer);
}

void VideoProcessingManager::unregister_observer(video::IFrameSourceObserver* observer)
{
    m_frame_observers.unregister_event_handler(observer);
}

}  // namespace step::proc