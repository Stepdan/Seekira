#pragma once

#include <core/base/types/meta_storage.hpp>
#include <core/task/base_task.hpp>
#include <core/threading/thread_pool_worker.hpp>

#include <video/frame/interfaces/frame_interfaces.hpp>

namespace step::proc {

struct VideoProcessorInfo
{
    video::Frame frame;
    MetaStorage storage;
};

using VideoProcessorInfoList = std::vector<VideoProcessorInfo>;
using VideoProcessorInfoPtr = std::shared_ptr<VideoProcessorInfo>;

class IVideoProcessorSourceObserver
{
public:
    virtual ~IVideoProcessorSourceObserver() = default;

    virtual void process_video_info(VideoProcessorInfoList) { STEP_UNDEFINED("process_video_info is undefined!"); }
};

class IVideoProcessorSource
{
public:
    virtual ~IVideoProcessorSource() = default;

    virtual void register_observer(IVideoProcessorSourceObserver* observer) = 0;
    virtual void unregister_observer(IVideoProcessorSourceObserver* observer) = 0;
};

template <typename TSettings>
using BaseVideoProcessorTask = task::BaseTask<TSettings, video::FramePtr, VideoProcessorInfoList>;
using IVideoProcessorTask = task::ITask<video::FramePtr, VideoProcessorInfoList>;
using VideoProcessorTaskPtr = std::unique_ptr<IVideoProcessorTask>();

using VideoProcessorId = std::string;

using IVideoProcessorObserver = threading::IThreadPoolWorkerEventObserver<std::string, VideoProcessorInfoList>;

class IVideoProcessor : public threading::ThreadPoolWorker<std::string, video::FramePtr, VideoProcessorInfoList>,
                        public IVideoProcessorSource,
                        public video::IFrameSourceObserver
{
public:
    virtual ~IVideoProcessor() = default;
};

}  // namespace step::proc