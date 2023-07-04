#pragma once

#include <proc/interfaces/detector_interface.hpp>

namespace step::proc {

std::unique_ptr<IDetector> create_face_detector(const std::shared_ptr<task::BaseSettings>& settings);

std::unique_ptr<IDetector> create_person_detector(const std::shared_ptr<task::BaseSettings>& settings);

}  // namespace step::proc