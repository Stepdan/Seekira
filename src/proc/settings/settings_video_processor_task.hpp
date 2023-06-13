#pragma once

#include <core/task/base_task.hpp>

#include <proc/interfaces/video_processor_interface.hpp>

namespace step::proc {

class SettingsVideoProcessorTask : public task::BaseSettings
{
public:
    TASK_SETTINGS(SettingsVideoProcessorTask)

    SettingsVideoProcessorTask() = default;

    bool operator==(const SettingsVideoProcessorTask& rhs) const noexcept;
    bool operator!=(const SettingsVideoProcessorTask& rhs) const noexcept { return !(*this == rhs); }

    void set_pipeline_config(const ObjectPtrJSON& cfg) { m_pipeline_cfg = cfg; }

    ObjectPtrJSON get_pipeline_cfg() const noexcept { return m_pipeline_cfg; }

public:
    ObjectPtrJSON m_pipeline_cfg;
};

}  // namespace step::proc