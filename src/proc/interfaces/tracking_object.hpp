#pragma once

#include <core/base/types/rect.hpp>

#include <memory>

namespace step::proc {

using TrackingObjectId = std::string;

class ITrackingObject
{
public:
    virtual ~ITrackingObject() = default;

    virtual void init(const TrackingObjectId& id, const Rect& bbox) = 0;

    virtual Rect get_bbox() const = 0;
    virtual const TrackingObjectId& get_tracking_id() const noexcept = 0;
};

using TrackingObjectPtr = std::shared_ptr<ITrackingObject>;

}  // namespace step::proc