#include "input_node.hpp"

#include <log/log.hpp>

namespace step::video::pipeline {

const std::string InputPipelineNode::INPUT_NODE_ID = "input_node";

const std::string InputNodeSettings::SETTINGS_ID = "InputNodeSettings";

void InputPipelineNode::process(InputPipelineNode::DataType data) { STEP_LOG(L_INFO, "Input pipeline node"); }

}  // namespace step::video::pipeline