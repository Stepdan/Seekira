#include "settings_face_detector.hpp"

namespace step::proc {

const std::string SettingsFaceDetector::SETTINGS_ID = "SettingsFaceDetector";

bool SettingsFaceDetector::operator==(const SettingsFaceDetector& rhs) const noexcept
{
    /* clang-format off */
    return true
        && m_alg_type == rhs.m_alg_type
        && m_model_path == rhs.m_model_path
    ;
    /* clang-format on */
}

}  // namespace step::proc