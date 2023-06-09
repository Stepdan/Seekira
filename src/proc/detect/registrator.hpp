#pragma once

#include <proc/interfaces/detector_interface.hpp>

namespace step::proc {

std::unique_ptr<IDetector> create_face_detector(const std::shared_ptr<task::BaseSettings>& settings);

}