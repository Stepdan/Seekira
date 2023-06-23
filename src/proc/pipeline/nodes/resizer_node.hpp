#pragma once

#include <core/log/log.hpp>

#include <proc/pipeline/pipeline_task.hpp>

#include <proc/settings/settings_resizer.hpp>

namespace step::proc {

class ResizerNodeSettings : public task::BaseSettings
{
public:
    TASK_SETTINGS(ResizerNodeSettings)

    ResizerNodeSettings() = default;

    bool operator==(const ResizerNodeSettings& rhs) const noexcept { return false; }
    bool operator!=(const ResizerNodeSettings& rhs) const noexcept { return !(*this == rhs); }

    std::shared_ptr<task::BaseSettings> get_resizer_settings_base() const noexcept { return m_settings; }

private:
    std::shared_ptr<task::BaseSettings> m_settings;
};

std::shared_ptr<task::BaseSettings> create_resizer_node_settings(const ObjectPtrJSON&);

std::unique_ptr<task::IAbstractTask> create_resizer_node(const std::shared_ptr<task::BaseSettings>& settings);

}  // namespace step::proc