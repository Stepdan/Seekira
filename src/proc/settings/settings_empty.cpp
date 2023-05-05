#include "settings_empty.hpp"

namespace step::proc {

const std::string SettingsEmpty::SETTINGS_ID = "SettingsEmpty";

std::shared_ptr<task::BaseSettings> create_settings_empty(const ObjectPtrJSON& cfg)
{
    return std::make_shared<SettingsEmpty>(cfg);
}

}  // namespace step::proc