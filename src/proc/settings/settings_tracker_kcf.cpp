#include "settings_tracker_kcf.hpp"

namespace step::proc {

const std::string SettingsTrackerKCF::SETTINGS_ID = "SettingsTrackerKCF";

bool SettingsTrackerKCF::operator==(const SettingsTrackerKCF& rhs) const noexcept
{
    /* clang-format off */
    return false
    ;
    /* clang-format on */
}

void SettingsTrackerKCF::deserialize(const ObjectPtrJSON& container) {}

std::shared_ptr<task::BaseSettings> create_tracker_kcf_settings(const ObjectPtrJSON& cfg)
{
    auto settings = std::make_shared<SettingsTrackerKCF>();
    settings->deserialize(cfg);

    return settings;
}

}  // namespace step::proc