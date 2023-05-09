#include "input_node.hpp"

namespace step::pipeline {

const std::string InputNodeSettings::SETTINGS_ID = "InputNodeSettings";

std::shared_ptr<task::BaseSettings> create_input_node_settings(const ObjectPtrJSON& cfg)
{
    return std::make_shared<InputNodeSettings>(cfg);
}

}  // namespace step::pipeline