#include "settings_empty.hpp"

namespace step::proc {

const std::string SettingsEmpty::SETTINGS_ID = "SettingsEmpty";

std::shared_ptr<task::BaseSettings> create_settings_empty(const ObjectPtrJSON& cfg)
{
    return std::make_shared<SettingsEmpty>(cfg);
}

void SettingsEmpty::deserialize(const ObjectPtrJSON& cfg) { STEP_UNDEFINED("SettingsEmpty::deserialize is undefined"); }

}  // namespace step::proc