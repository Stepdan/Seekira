#pragma once

#include <core/log/log.hpp>

#include <proc/pipeline/pipeline_task.hpp>

#include <proc/drawer/settings_drawer.hpp>

namespace step::proc {

class DrawerNodeSettings : public task::BaseSettings
{
public:
    TASK_SETTINGS(DrawerNodeSettings)

    DrawerNodeSettings() = default;

    bool operator==(const DrawerNodeSettings& rhs) const noexcept { return false; }
    bool operator!=(const DrawerNodeSettings& rhs) const noexcept { return !(*this == rhs); }

    SettingsDrawer get_drawer_settings() const noexcept { return m_drawer_settings; }

private:
    SettingsDrawer m_drawer_settings;
};

std::shared_ptr<task::BaseSettings> create_drawer_node_settings(const ObjectPtrJSON&);

std::unique_ptr<task::IAbstractTask> create_drawer_node(const std::shared_ptr<task::BaseSettings>& settings);

}  // namespace step::proc