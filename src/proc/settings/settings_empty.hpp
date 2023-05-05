#pragma once

#include <abstract_task/base_task.hpp>

namespace step::proc {

class SettingsEmpty : public task::BaseSettings
{
public:
    TASK_SETTINGS(SettingsEmpty)

    SettingsEmpty() = default;

    bool operator==(const SettingsEmpty& rhs) const noexcept { return true; }
    bool operator!=(const SettingsEmpty& rhs) const noexcept { return !(*this == rhs); }
};

std::shared_ptr<task::BaseSettings> create_settings_empty(const ObjectPtrJSON&);

}  // namespace step::proc