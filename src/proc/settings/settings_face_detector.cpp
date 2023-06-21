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
        && m_face_engine_init == rhs.m_face_engine_init
        && m_face_engine_conn_id == rhs.m_face_engine_conn_id
    ;
    /* clang-format on */
}

void SettingsFaceDetector::deserialize(const ObjectPtrJSON& container)
{
    auto face_engine_conn_id_opt = json::get_opt<std::string>(container, CFG_FLD::FACE_ENGINE_CONNECTION_ID);
    if (face_engine_conn_id_opt.has_value())
    {
        m_face_engine_conn_id = face_engine_conn_id_opt.value();
        STEP_ASSERT(!m_face_engine_conn_id.empty(), "FACE_ENGINE_CONNECTION_ID can't be empty!");
        return;
    }
}

std::shared_ptr<task::BaseSettings> create_face_detector_settings(const ObjectPtrJSON& cfg)
{
    auto settings = std::make_shared<SettingsFaceDetector>();
    settings->deserialize(cfg);

    return settings;
}

}  // namespace step::proc