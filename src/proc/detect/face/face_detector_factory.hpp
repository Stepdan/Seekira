#pragma once

#include <proc/detect/registrator.hpp>

namespace step::proc {

std::unique_ptr<IDetector> create_face_detector_tdv(const std::shared_ptr<task::BaseSettings>& settings);

}