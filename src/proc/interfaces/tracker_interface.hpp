#pragma once

#include <core/log/log.hpp>

#include <core/task/base_task.hpp>

#include <video/frame/interfaces/frame.hpp>

#include "tracking_object.hpp"

#include <robin_hood.h>

namespace step::proc {

class ITrackerFunctionality
{
public:
    virtual ~ITrackerFunctionality() = default;

    virtual void add_object(const TrackingObjectPtr& obj) = 0;

    virtual void remove_object(const TrackingObjectId& id) = 0;
};

using ITracker = task::ITask<video::Frame&, void, step::proc::ITrackerFunctionality>;

using TrackerPtr = std::unique_ptr<ITracker>;

template <typename TSettings>
class BaseTracker : virtual public task::BaseTask<TSettings, video::Frame&, void, step::proc::ITrackerFunctionality>
{
public:
    void add_object(const TrackingObjectPtr& obj) override
    {
        STEP_ASSERT(obj, "Empty tracking obj!");
        const auto& track_id = obj->get_tracking_id();
        STEP_ASSERT(!m_objects.contains(track_id), "Tracking object {} has been already added to tracking engine!",
                    track_id);
        m_objects[track_id] = obj;
    }

    void remove_object(const TrackingObjectId& track_id) override
    {
        if (!m_objects.contains(track_id))
            STEP_LOG(L_WARN, "Tracking object {} has been removed already from tracking engine!", track_id);

        m_objects.erase(track_id);
    }

private:
    robin_hood::unordered_map<TrackingObjectId, TrackingObjectPtr> m_objects;
};

}  // namespace step::proc