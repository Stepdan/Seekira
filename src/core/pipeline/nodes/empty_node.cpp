#include "empty_node.hpp"

namespace step::pipeline {

const std::string EmptyNodeSettings::SETTINGS_ID = "EmptyNodeSettings";

std::shared_ptr<task::BaseSettings> create_empty_node_settings(const ObjectPtrJSON& cfg)
{
    return std::make_shared<EmptyNodeSettings>(cfg);
}

}  // namespace step::pipeline