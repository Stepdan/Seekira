#include "video_processor_task.hpp"

#include <proc/pipeline/impl/frame_pipeline.hpp>

#include <proc/settings/settings_video_processor_task.hpp>

namespace step::proc {

class VideoProcessorTask : public BaseVideoProcessorTask<SettingsVideoProcessorTask>
{
public:
    VideoProcessorTask(const std::shared_ptr<task::BaseSettings>& settings)
    {
        set_settings(*settings);
        m_frame_pipeline = FrameSyncPipeline::create(m_typed_settings.get_pipeline_cfg());
    }

    VideoProcessorInfo process(video::FramePtr frame)
    {
        STEP_ASSERT(frame, "Invalid frame!");

        VideoProcessorInfo result;

        {
            auto data = VideoProcessorInfo::create(std::move(*frame));

            m_frame_pipeline->process(data);

            std::swap(result, *data);
        }

        return result;
    }

private:
    std::unique_ptr<FrameSyncPipeline> m_frame_pipeline;
};

std::unique_ptr<IVideoProcessorTask> create_video_processor_task(const std::shared_ptr<task::BaseSettings>& settings)
{
    return std::make_unique<VideoProcessorTask>(settings);
}

}  // namespace step::proc