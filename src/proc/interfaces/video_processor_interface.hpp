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

class IVideoProcessorSourceObserver
{
public:
    virtual ~IVideoProcessorSourceObserver() = default;

    virtual void process_video_info(VideoProcessorInfoPtr) { STEP_UNDEFINED("process_video_info is undefined!"); }
};

class IVideoProcessorSource
{
public:
    virtual ~IVideoProcessorSource() = default;

    virtual void register_observer(IVideoProcessorSourceObserver* observer) = 0;
    virtual void unregister_observer(IVideoProcessorSourceObserver* observer) = 0;
};

using IVideoProcessorTask = task::ITask<video::FramePtr, VideoProcessorInfo>;
using VideoProcessorTaskPtr = std::unique_ptr<IVideoProcessorTask>();

template <typename TSettings>
using BaseVideoProcessorTask = task::BaseTask<TSettings, video::FramePtr, VideoProcessorInfo>;

using VideoProcessorId = std::string;

using IVideoProcessorObserver = threading::IThreadPoolWorkerEventObserver<VideoProcessorId, VideoProcessorInfoPtr>;

class IVideoProcessor : public threading::ThreadPoolWorker<VideoProcessorId, video::FramePtr, VideoProcessorInfo>,
                        public IVideoProcessorSource,
                        public video::IFrameSourceObserver
{
public:
    virtual ~IVideoProcessor() = default;
};

}  // namespace step::proc