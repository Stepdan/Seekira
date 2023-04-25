#include "node_graph.hpp"

#include <base/utils/json/json_utils.hpp>

#include <abstract_task/settings_factory.hpp>
#include <abstract_task/abstract_task_factory.hpp>

namespace step::video::pipeline {

FramePipelineGraphNode::FramePipelineGraphNode(const std::string& node_id, std::unique_ptr<IFramePipelineNode>&& node)
    : m_node_id(node_id), m_node(std::move(node))
{
}

FramePipelineGraphNode::FramePipelineGraphNode(const ObjectPtrJSON& json_obj) { deserialize(json_obj); }

void FramePipelineGraphNode::deserialize(const ObjectPtrJSON& container)
{
    m_node_id = json::get<std::string>(container, CFG_FLD::NODE);

    const auto task_settings = json::get_object(container, CFG_FLD::SETTINGS);
    const auto base_task_settings = task::TaskSettingsFactory::instance().create(task_settings);

    m_node = task::AbstractTaskFactory<FramePipelineDataType&>::instance().create_unique(m_node_id, base_task_settings);
}

std::optional<std::string> FramePipelineGraphNode::get_parent_id_opt() const noexcept
{
    if (!m_parent)
        return std::nullopt;

    return m_parent->get_id();
}

std::vector<std::string> FramePipelineGraphNode::get_sinks_id() const
{
    std::vector<std::string> sink_ids;
    for (auto& sink : m_sinks)
        sink_ids.push_back(sink->get_id());

    return sink_ids;
}

void FramePipelineGraphNode::set_activate_sink_callback(PipelineActivateSinkCallback callback)
{
    m_activate_sink_callback = callback;
}

void FramePipelineGraphNode::set_node_finished_callback(PipelineNodeFinishedCallback callback)
{
    m_finish_callback = callback;
}

void FramePipelineGraphNode::process(FramePipelineDataType&& data)
{
    STEP_ASSERT(m_finish_callback, "No finish callback for node {}", m_node_id);
    STEP_ASSERT(m_activate_sink_callback, "No activate sink callback for node {}", m_node_id);

    auto process_data = std::move(data);
    m_node->process(process_data);

    if (m_sinks.empty())
    {
        m_finish_callback(m_node_id);
        return;
    }

    if (m_sinks.size() == 1)
    {
        m_sinks.front()->process(std::move(process_data));
        return;
    }
    else
    {
        for (size_t i = 0; m_sinks.size(); ++i)
            m_activate_sink_callback(m_sinks[i]->get_id(), process_data.clone());

        m_finish_callback(m_node_id);
    }
}

}  // namespace step::video::pipeline