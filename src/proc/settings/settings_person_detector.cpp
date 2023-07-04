#include "settings_person_detector.hpp"

#include <core/base/types/config_fields.hpp>

#include <core/base/utils/string_utils.hpp>

#include <proc/interfaces/face_engine.hpp>

namespace step::proc {

const std::string SettingsPersonDetector::SETTINGS_ID = "SettingsPersonDetector";

bool SettingsPersonDetector::operator==(const SettingsPersonDetector& rhs) const noexcept
{
    /* clang-format off */
    return true
        && m_model_path == rhs.m_model_path
    ;
    /* clang-format on */
}

void SettingsPersonDetector::deserialize(const ObjectPtrJSON& container)
{
    m_model_path = json::get<std::string>(container, CFG_FLD::MODEL_PATH);
}

std::shared_ptr<task::BaseSettings> create_person_detector_settings(const ObjectPtrJSON& cfg)
{
    auto settings = std::make_shared<SettingsPersonDetector>();
    settings->deserialize(cfg);

    return settings;
}

}  // namespace step::proc