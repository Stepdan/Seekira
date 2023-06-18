#pragma once

#include <core/task/base_task.hpp>

#include <video/frame/interfaces/frame.hpp>

#include <proc/pipeline/pipeline_data.hpp>

namespace step::proc {

using VideoProcessorInfo = PipelineData<video::Frame>;
using VideoProcessorInfoPtr = std::shared_ptr<VideoProcessorInfo>;

using IVideoProcessorTask = task::ITask<video::FramePtr, VideoProcessorInfo>;
using VideoProcessorTaskPtr = std::unique_ptr<IVideoProcessorTask>;

template <typename TSettings>
using BaseVideoProcessorTask = task::BaseTask<TSettings, video::FramePtr, VideoProcessorInfo>;

}  // namespace step::proc