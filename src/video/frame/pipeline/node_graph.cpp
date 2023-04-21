#include "node_graph.hpp"

#include <base/utils/json/json_utils.hpp>

#include <abstract_task/settings_factory.hpp>
#include <abstract_task/abstract_task_factory.hpp>

namespace {

constexpr std::string_view NODE_FIELD = "node";
constexpr std::string_view SETTINGS_FIELD = "settings";

}  // namespace

namespace step::video::pipeline {

FramePipelineGraphNode::FramePipelineGraphNode(const std::string& node_id, std::unique_ptr<IFramePipelineNode>&& node,
                                               PipelineActivateThreadCallback callback)
    : m_node_id(node_id), m_node(std::move(node)), m_activate_thread_callback(callback)
{
}

FramePipelineGraphNode::FramePipelineGraphNode(const ObjectPtrJSON& json_obj, PipelineActivateThreadCallback callback)
    : m_activate_thread_callback(callback)
{
    deserialize(json_obj);
}

void FramePipelineGraphNode::deserialize(const ObjectPtrJSON& container)
{
    m_node_id = json::get<std::string>(container, NODE_FIELD.data());

    const auto task_settings = json::get_object(container, SETTINGS_FIELD.data());
    const auto base_task_settings = task::TaskSettingsFactory::instance().create(task_settings);

    m_node = task::AbstractTaskFactory<FramePipelineDataType>::instance().create_unique(m_node_id, base_task_settings);
}

void FramePipelineGraphNode::process(FramePipelineDataType&& data)
{
    auto process_data = std::move(data);
    m_node->process(process_data);

    if (m_sinks.empty())
        return;

    if (m_sinks.size() > 1)
    {
        for (size_t i = 1; m_sinks.size(); ++i)
            m_activate_thread_callback(m_sinks[i]->get_id(), process_data.clone());
    }

    m_sinks.front()->process(std::move(process_data));
}

}  // namespace step::video::pipeline