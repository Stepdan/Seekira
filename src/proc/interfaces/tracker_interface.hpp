#pragma once

#include <core/task/base_task.hpp>

#include <video/frame/interfaces/frame.hpp>

#include "tracking_object.hpp"

namespace step::proc {

struct TrackingData
{
    video::Frame& frame;
    TrackingObjectPtr object;
};

using ITracker = task::ITask<TrackingData>;
using TrackerPtr = std::unique_ptr<ITracker>;

template <typename TSettings>
using BaseTracker = task::BaseTask<TSettings, TrackingData>;

}  // namespace step::proc