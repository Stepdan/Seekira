#include "exception_node.hpp"

namespace step::pipeline {

const std::string ExceptionNodeSettings::SETTINGS_ID = "ExceptionNodeSettings";

std::shared_ptr<task::BaseSettings> create_exception_node_settings(const ObjectPtrJSON& cfg)
{
    return std::make_shared<ExceptionNodeSettings>(cfg);
}

}  // namespace step::pipeline