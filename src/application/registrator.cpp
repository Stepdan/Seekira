#include "registrator.hpp"

#include <core/task/settings_factory.hpp>
#include <core/task/task_factory.hpp>

#include <video/frame/interfaces/frame.hpp>

#include <proc/detect/registrator.hpp>
#include <proc/pipeline/nodes/empty_node.hpp>
#include <proc/pipeline/nodes/exception_node.hpp>
#include <proc/pipeline/nodes/input_node.hpp>

namespace step::app {

Registrator& Registrator::instance()
{
    static Registrator obj;
    return obj;
}

Registrator::Registrator()
{
    /* clang-format off */
    REGISTER_TASK_SETTINGS_CREATOR(proc::InputNodeSettings::SETTINGS_ID, &proc::create_input_node_settings);
    REGISTER_TASK_SETTINGS_CREATOR(proc::EmptyNodeSettings::SETTINGS_ID, &proc::create_empty_node_settings);
    REGISTER_TASK_SETTINGS_CREATOR(proc::ExceptionNodeSettings::SETTINGS_ID, &proc::create_exception_node_settings);

    //REGISTER_TASK_SETTINGS_CREATOR(proc::SettingsFaceDetector::SETTINGS_ID, &proc::create_face_detector);

    REGISTER_TASK_CREATOR_UNIQUE(proc::PipelineDataPtr<video::Frame>, proc::InputNodeSettings::SETTINGS_ID, &proc::create_input_node<video::Frame>);
    REGISTER_TASK_CREATOR_UNIQUE(proc::PipelineDataPtr<video::Frame>, proc::EmptyNodeSettings::SETTINGS_ID, &proc::create_empty_node<video::Frame>);
    REGISTER_TASK_CREATOR_UNIQUE(proc::PipelineDataPtr<video::Frame>, proc::ExceptionNodeSettings::SETTINGS_ID, &proc::create_exception_node<video::Frame>);

    /* clang-format on */
}

}  // namespace step::app