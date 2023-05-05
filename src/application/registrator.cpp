#include "registrator.hpp"

#include <pipeline/nodes/empty_node.hpp>
#include <pipeline/nodes/exception_node.hpp>
#include <pipeline/nodes/input_node.hpp>

#include <abstract_task/settings_factory.hpp>
#include <abstract_task/abstract_task_factory.hpp>

#include <video/frame/interfaces/frame.hpp>

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

    REGISTER_ABSTRACT_TASK_CREATOR_UNIQUE(std::shared_ptr<pipeline::PipelineData<video::Frame>>, pipeline::InputNodeSettings::SETTINGS_ID, &pipeline::create_input_node<video::Frame>);
    REGISTER_ABSTRACT_TASK_CREATOR_UNIQUE(std::shared_ptr<pipeline::PipelineData<video::Frame>>, pipeline::EmptyNodeSettings::SETTINGS_ID, &pipeline::create_empty_node<video::Frame>);
    REGISTER_ABSTRACT_TASK_CREATOR_UNIQUE(std::shared_ptr<pipeline::PipelineData<video::Frame>>, pipeline::ExceptionNodeSettings::SETTINGS_ID, &pipeline::create_exception_node<video::Frame>);
    /* clang-format on */
}

}  // namespace step::app