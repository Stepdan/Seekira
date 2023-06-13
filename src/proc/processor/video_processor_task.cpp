#include "video_processor_task.hpp"

#include <proc/pipeline/frame_pipeline.hpp>

#include <proc/settings/settings_video_processor_task.hpp>

namespace step::proc {

class VideoProcessorTask
    : public BaseVideoProcessorTask<SettingsVideoProcessorTask>,
      public pipeline::IPipelineEventObserver<std::shared_ptr<pipeline::PipelineData<video::Frame>>>
{
public:
    ~VideoProcessorTask()
    {
        m_need_iterrupt = true;
        m_data_cnd.notify_one();
    }

public:
    VideoProcessorTask(const std::shared_ptr<task::BaseSettings>& settings)
    {
        set_settings(*settings);
        m_frame_pipeline = std::make_unique<FramePipeline>(m_typed_settings.get_pipeline_cfg());
        m_frame_pipeline->register_observer(this);
    }

    VideoProcessorInfoList process(video::FramePtr frame)
    {
        STEP_ASSERT(frame, "Invalid frame!");

        m_frame_pipeline->process_frame(frame);

        std::unique_lock lock(m_data_guard);
        m_data_cnd.wait(lock, [this]() { return m_has_data && m_need_iterrupt; });

        if (m_need_iterrupt)
            return {};

        m_has_data = false;
        return m_result_list;
    }

    void on_pipeline_data_update(
        const pipeline::PipelineResultMap<std::shared_ptr<pipeline::PipelineData<video::Frame>>>& data)
    {
        std::scoped_lock lock(m_data_guard);
        m_result_list.clear();
        for (auto& [id, pipeline_data] : data)
        {
            VideoProcessorInfo info;
            info.frame = pipeline_data->data;
            info.storage = pipeline_data->storage;
            m_result_list.push_back(info);
        }
        m_has_data = true;
        m_data_cnd.notify_one();
    }

private:
    std::unique_ptr<FramePipeline> m_frame_pipeline;

    VideoProcessorInfoList m_result_list;
    std::atomic_bool m_has_data{false};
    std::atomic_bool m_need_iterrupt{false};
    std::mutex m_data_guard;
    std::condition_variable m_data_cnd;
};

std::unique_ptr<IVideoProcessorTask> create_video_processor_task(const std::shared_ptr<task::BaseSettings>& settings)
{
    return nullptr;
}

}  // namespace step::proc