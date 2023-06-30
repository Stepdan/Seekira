#include "registrator.hpp"

#include <proc/tracking/opencv/registrator.hpp>

namespace step::proc {

TrackerPtr create_tracker_kcf(const std::shared_ptr<task::BaseSettings>& settings)
{
    return create_tracker_kcf_opencv(settings);
}

}  // namespace step::proc