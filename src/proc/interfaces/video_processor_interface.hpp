#pragma once

#include <core/base/types/meta_storage.hpp>
#include <core/task/base_task.hpp>
#include <core/threading/thread_pool_worker.hpp>

#include <video/frame/interfaces/frame_interfaces.hpp>

#include <proc/pipeline/pipeline_data.hpp>

namespace step::proc {

using VideoProcessorInfo = PipelineData<video::Frame>;
using VideoProcessorInfoList = std::vector<VideoProcessorInfo>;
using VideoProcessorInfoPtr = std::shared_ptr<VideoProcessorInfo>;

using IVideoProcessorTask = task::ITask<video::FramePtr, VideoProcessorInfo>;
using VideoProcessorTaskPtr = std::unique_ptr<IVideoProcessorTask>;

template <typename TSettings>
using BaseVideoProcessorTask = task::BaseTask<TSettings, video::FramePtr, VideoProcessorInfo>;

using VideoProcessorId = std::string;

using IVideoProcessorObserver = threading::IThreadPoolWorkerEventObserver<VideoProcessorId, VideoProcessorInfo>;

class IVideoProcessor : public threading::ThreadPoolWorker<VideoProcessorId, video::FramePtr, VideoProcessorInfo>,
                        public video::IFrameSourceObserver
{
    using ThreadPoolWorkerType = threading::ThreadPoolWorker<VideoProcessorId, video::FramePtr, VideoProcessorInfo>;

public:
    virtual ~IVideoProcessor() = default;

protected:
    IVideoProcessor(const VideoProcessorId& id) : ThreadPoolWorkerType(id) {}
};

}  // namespace step::proc