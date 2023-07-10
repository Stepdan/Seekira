#include "settings_person_detector.hpp"

#include <core/base/types/config_fields.hpp>

#include <core/base/utils/string_utils.hpp>

#include <proc/interfaces/face_engine.hpp>

namespace step::proc {

const std::string SettingsPersonDetector::SETTINGS_ID = "SettingsPersonDetector";

bool SettingsPersonDetector::operator==(const SettingsPersonDetector& rhs) const noexcept
{
    /* clang-format off */
    return false
    ;
    /* clang-format on */
}

void SettingsPersonDetector::deserialize(const ObjectPtrJSON& container)
{
    m_neural_net_cfg = json::get_object(container, CFG_FLD::NEURAL_NET_SETTINGS);
    m_resizer_cfg = json::get_object(container, CFG_FLD::RESIZER_SETTINGS);
}

std::shared_ptr<task::BaseSettings> create_person_detector_settings(const ObjectPtrJSON& cfg)
{
    auto settings = std::make_shared<SettingsPersonDetector>();
    settings->deserialize(cfg);

    return settings;
}

}  // namespace step::proc