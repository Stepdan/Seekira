#include "settings_video_processor_task.hpp"

#include <core/base/types/config_fields.hpp>

namespace step::proc {

const std::string SettingsVideoProcessorTask::SETTINGS_ID = "SettingsVideoProcessorTask";

bool SettingsVideoProcessorTask::operator==(const SettingsVideoProcessorTask& rhs) const noexcept
{
    /* clang-format off */
    return false
    ;
    /* clang-format on */
}

void SettingsVideoProcessorTask::deserialize(const ObjectPtrJSON& cfg)
{
    m_pipeline_cfg = json::get_object(cfg, CFG_FLD::PIPELINE);
    STEP_ASSERT(m_pipeline_cfg, "Invalid pipeline cfg!");
}

std::shared_ptr<task::BaseSettings> create_video_processor_task_settings(const ObjectPtrJSON& cfg)
{
    auto settings = std::make_shared<SettingsVideoProcessorTask>();
    settings->deserialize(cfg);

    return settings;
}

}  // namespace step::proc