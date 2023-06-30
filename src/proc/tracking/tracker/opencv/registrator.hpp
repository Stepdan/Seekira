#pragma once

#include <proc/interfaces/tracker_interface.hpp>

namespace step::proc {

TrackerPtr create_tracker_kcf_opencv(const std::shared_ptr<task::BaseSettings>& settings);

}