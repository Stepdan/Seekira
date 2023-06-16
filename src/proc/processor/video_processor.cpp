#include "video_processor.hpp"

#include <core/base/types/config_fields.hpp>

#include <core/task/settings_factory.hpp>
#include <core/task/task_factory.hpp>

namespace step::proc {

std::unique_ptr<IVideoProcessor> VideoProcessor::create(const ObjectPtrJSON& config)
{
    return std::make_unique<VideoProcessor>(config);
}

VideoProcessor::VideoProcessor(const ObjectPtrJSON& config)
    : IVideoProcessor([&config]() { return json::get<std::string>(config, CFG_FLD::ID); }())
{
    deserialize(config);
}

VideoProcessor::~VideoProcessor() { stop_worker(); }

VideoProcessorInfo VideoProcessor::thread_pool_worker_process_data(const video::FramePtr& data)
{
    STEP_ASSERT(data, "Invalid frame in thread_pool_worker_process_data VideoProcessor {}", get_id());
    STEP_ASSERT(m_task, "Invalid task in thread_pool_worker_process_data VideoProcessor {}", get_id());
    return m_task->process(data);
}

void VideoProcessor::process_frame(video::FramePtr frame)
{
    STEP_ASSERT(frame, "Invalid frame in process_frame VideoProcessor {}", get_id());
    add_data(std::move(frame));
}

void VideoProcessor::deserialize(const ObjectPtrJSON& container)
{
    auto config_json = json::get_object(container, CFG_FLD::CONFIG);
    m_task = IVideoProcessorTask::from_abstract(CREATE_TASK_UNIQUE(CREATE_SETTINGS(config_json)));
}

}  // namespace step::proc