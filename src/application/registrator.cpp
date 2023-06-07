#include "registrator.hpp"

#include <core/pipeline/nodes/empty_node.hpp>
#include <core/pipeline/nodes/exception_node.hpp>
#include <core/pipeline/nodes/input_node.hpp>

#include <core/task/settings_factory.hpp>
#include <core/task/task_factory.hpp>

#include <video/frame/interfaces/frame.hpp>

#include <proc/detect/registrator.hpp>

namespace step::app {

Registrator& Registrator::instance()
{
    static Registrator obj;
    return obj;
}

Registrator::Registrator()
{
    /* clang-format off */
    REGISTER_TASK_SETTINGS_CREATOR(pipeline::InputNodeSettings::SETTINGS_ID, &pipeline::create_input_node_settings);
    REGISTER_TASK_SETTINGS_CREATOR(pipeline::EmptyNodeSettings::SETTINGS_ID, &pipeline::create_empty_node_settings);
    REGISTER_TASK_SETTINGS_CREATOR(pipeline::ExceptionNodeSettings::SETTINGS_ID, &pipeline::create_exception_node_settings);

    //REGISTER_TASK_SETTINGS_CREATOR(proc::SettingsFaceDetector::SETTINGS_ID, &proc::create_face_detector);

    REGISTER_TASK_CREATOR_UNIQUE(pipeline::PipelineDataTypePtr<video::Frame>, pipeline::InputNodeSettings::SETTINGS_ID, &pipeline::create_input_node<video::Frame>);
    REGISTER_TASK_CREATOR_UNIQUE(pipeline::PipelineDataTypePtr<video::Frame>, pipeline::EmptyNodeSettings::SETTINGS_ID, &pipeline::create_empty_node<video::Frame>);
    REGISTER_TASK_CREATOR_UNIQUE(pipeline::PipelineDataTypePtr<video::Frame>, pipeline::ExceptionNodeSettings::SETTINGS_ID, &pipeline::create_exception_node<video::Frame>);

    /* clang-format on */
}

}  // namespace step::app