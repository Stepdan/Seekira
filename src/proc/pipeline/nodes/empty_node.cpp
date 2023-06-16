#include "empty_node.hpp"

namespace step::proc {

const std::string EmptyNodeSettings::SETTINGS_ID = "EmptyNodeSettings";

std::shared_ptr<task::BaseSettings> create_empty_node_settings(const ObjectPtrJSON& cfg)
{
    return std::make_shared<EmptyNodeSettings>(cfg);
}

void EmptyNodeSettings::deserialize(const ObjectPtrJSON& cfg)
{
    STEP_UNDEFINED("EmptyNodeSettings::deserialize is undefined");
}

}  // namespace step::proc