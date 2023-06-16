#include "settings_face_detector.hpp"

#include <core/base/types/config_fields.hpp>

#include <core/base/utils/string_utils.hpp>

#include <proc/interfaces/face_engine.hpp>

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

void SettingsFaceDetector::deserialize(const ObjectPtrJSON& container)
{
    m_model_path = json::get<std::string>(container, CFG_FLD::MODEL_PATH);
    step::utils::from_string<FaceEngineType>(m_face_engine_type, json::get<std::string>(container, CFG_FLD::TYPE));
    step::utils::from_string<IFaceEngine::Mode>(m_mode, json::get<std::string>(container, CFG_FLD::MODE));
}

std::shared_ptr<task::BaseSettings> create_face_detector_settings(const ObjectPtrJSON& cfg)
{
    auto settings = std::make_shared<SettingsFaceDetector>();
    settings->deserialize(cfg);

    return settings;
}

}  // namespace step::proc