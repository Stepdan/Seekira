#include "settings_face_detector.hpp"

namespace step::proc {

bool SettingsFaceDetector::operator==(const SettingsFaceDetector& rhs) const noexcept
{
    /* clang-format off */
    return true
        && m_alg_type == rhs.m_alg_type
    ;
    /* clang-format on */
}

}  // namespace step::proc