#pragma once

#include <proc/interfaces/detector_interface.hpp>

#include <proc/settings/settings_face_detector.hpp>

namespace step::proc {

std::unique_ptr<IDetector> create_face_detector(const std::shared_ptr<task::BaseSettings>& settings);

}