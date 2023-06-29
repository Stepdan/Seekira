#pragma once

#include <core/task/base_task.hpp>

namespace step::proc {

class SettingsTrackerKCF : public task::BaseSettings
{
public:
    TASK_SETTINGS(SettingsTrackerKCF)

    SettingsTrackerKCF() = default;

    bool operator==(const SettingsTrackerKCF& rhs) const noexcept;
    bool operator!=(const SettingsTrackerKCF& rhs) const noexcept { return !(*this == rhs); }
};

std::shared_ptr<task::BaseSettings> create_tracker_kcf_settings(const ObjectPtrJSON&);

}  // namespace step::proc