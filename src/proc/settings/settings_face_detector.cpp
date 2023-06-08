#include "settings_face_detector.hpp"

namespace step::proc {

const std::string SettingsFaceDetector::SETTINGS_ID = "SettingsFaceDetector";

bool SettingsFaceDetector::operator==(const SettingsFaceDetector& rhs) const noexcept
{
    /* clang-format off */
    return true
        && m_face_engine_type == rhs.m_face_engine_type
        && m_model_path == rhs.m_model_path
        && m_mode == rhs.m_mode
    ;
    /* clang-format on */
}

}  // namespace step::proc