#pragma once

#include "tracker_interface.hpp"

#include <core/log/log.hpp>
#include <core/task/base_task.hpp>

namespace step::proc {

class ITrackingEngine
{
public:
    using TrackObjectMap = robin_hood::unordered_map<TrackingObjectId, TrackingObjectPtr>;

public:
    virtual ~ITrackingEngine() = default;

    virtual void add_object(const TrackingObjectPtr& obj) = 0;
    virtual void remove_object(const TrackingObjectId& id) = 0;

    virtual const TrackingObjectPtr& get_object(const TrackingObjectId& id) const noexcept = 0;
};

class BaseTrackingEngine : public ITrackingEngine
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
        {
            STEP_LOG(L_WARN, "Tracking object {} has been removed already from tracking engine!", track_id);
            return;
        }

        m_objects.erase(track_id);
    }

    const TrackingObjectPtr& get_object(const TrackingObjectId& track_id) const noexcept override
    {
        if (!m_objects.contains(track_id))
        {
            STEP_LOG(L_WARN, "Tracking engine doesn't have tracking object {}!", track_id);
            return nullptr;
        }

        return m_objects[track_id];
    }

private:
    TrackObjectMap m_objects;
};

}  // namespace step::proc