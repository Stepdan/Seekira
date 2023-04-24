#include "pipeline.hpp"

#include <video/frame/pipeline/nodes/input_node.hpp>

#include <base/utils/exception/assert.hpp>

namespace {

constexpr std::string_view INPUT_NODE_ID = "input_node";

constexpr std::string_view CFG_FLD_PIPELINE = "pipeline";
constexpr std::string_view CFG_FLD_PIPELINE_SETTINGS = "settings";
constexpr std::string_view CFG_FLD_PIPELINE_NODES = "nodes";
constexpr std::string_view CFG_FLD_PIPELINE_LINKS = "links";

}  // namespace

namespace step::video::pipeline {

FramePipeline::FramePipeline(const ObjectPtrJSON& config) { deserialize(config); }

void FramePipeline::deserialize(const ObjectPtrJSON& config)
{
    auto pipeline_json = json::get_object(config, CFG_FLD_PIPELINE.data());

    auto settings_json = json::get_object(config, CFG_FLD_PIPELINE_SETTINGS.data());
    m_settings = std::make_shared<FramePipelineSettings>(settings_json);
    const auto& pipeline_name = m_settings->get_pipeline_name();

    /* clang-format off */
    std::unordered_map<std::string, FramePipelineGraphNode::Ptr> nodes;
    auto nodes_collection = json::get_array(pipeline_json, CFG_FLD_PIPELINE_NODES.data());
    json::for_each_in_array<ObjectPtrJSON>(nodes_collection, [&pipeline_name, &nodes](const ObjectPtrJSON& node_cfg)
    {
        auto node = std::make_shared<FramePipelineGraphNode>(node_cfg);
        STEP_ASSERT(nodes.insert({node->get_id(), node}).second, "Pipeline {}: node {} already exists!", pipeline_name,
                    node->get_id());
    });

    std::vector<std::pair<std::string, std::string>> links;
    auto links_collection = json::get_array(pipeline_json, CFG_FLD_PIPELINE_LINKS.data());
    json::for_each_in_array<ArrayPtrJSON>(links_collection, [&pipeline_name, &links, &nodes](const ArrayPtrJSON& link_cfg)
    {
        STEP_ASSERT(link_cfg->size() == 2, "Pipeline {}: Invalid link", pipeline_name);
        const auto parent_id = json::get_array_value<std::string>(link_cfg, 0);
        const auto child_id = json::get_array_value<std::string>(link_cfg, 1);

        STEP_ASSERT(nodes.contains(parent_id), "Pipeline {}: link process error: no parent with id {}", pipeline_name, parent_id);
        STEP_ASSERT(nodes.contains(child_id), "Pipeline {}: link process error: no parent with id {}", pipeline_name, child_id);
        STEP_ASSERT(parent_id != child_id, "Pipeline {}: link process error: parent = child = {}", pipeline_name, parent_id);

        links.emplace_back(parent_id, child_id);
    });
    /* clang-format on */

    for (const auto& [parent_id, child_id] : links)
    {
        auto parent = nodes[parent_id];
        auto child = nodes[child_id];
        {
            const auto parent_id_opt = child->get_parent_id_opt();
            STEP_ASSERT(parent_id_opt.has_value(),
                        "Pipeline {}: child node {} already has parent {} but new parent {} are proposed",
                        pipeline_name, child_id, parent_id_opt.value(), parent_id);
        }

        child->set_parent(parent);
        parent->add_sink(child);
    }

    // Find root node (ALWAYS should be InputPipelineNode)
    STEP_ASSERT(nodes.contains(InputPipelineNode::INPUT_NODE_ID), "Pipeline {}: no input node!", pipeline_name);

    m_root = nodes[InputPipelineNode::INPUT_NODE_ID];

    {
        const auto root_parent_opt = m_root->get_parent_id_opt();
        STEP_ASSERT(!root_parent_opt.has_value(), "Pipeline {}:, input node has parent {}", pipeline_name,
                    root_parent_opt.value());
    }

    // Init thread pool
    m_thread_pool = std::make_unique<PipelineThreadPool>();
    m_thread_pool->set_pipeline_settings(m_settings);

    m_thread_pool->add_thread(std::make_unique<PipelineThread>(m_root));

    for (auto [id, node_ptr] : nodes)
    {
        const auto sink_ids = node_ptr->get_sinks_id();
        if (sink_ids.size() == 1)
            continue;

        if (sink_ids.empty())
        {
            node_ptr->set_node_finished_callback([this](const std::string& id) {
                STEP_ASSERT(is_thread_pool_exist(), "No thread pool existed in node finished callback for node {}", id);
                m_thread_pool->stop(id);
            });
        }

        if (sink_ids.size() > 1)
        {
            for (const auto& sink_id : sink_ids)
            {
                STEP_ASSERT(nodes.contains(sink_id), "No sink node with id {} for node {}", sink_id, id);
                m_thread_pool->add_thread(std::make_unique<PipelineThread>(nodes[sink_id]));
            }

            node_ptr->set_activate_sink_callback([this](const std::string& id, FramePipelineDataType&& data) {
                STEP_ASSERT(m_thread_pool, "No thread pool existed in activate sink callback for node {}", id);
                m_thread_pool->run(id, std::move(data));
            });

            node_ptr->set_node_finished_callback([this](const std::string& id) {
                STEP_ASSERT(m_thread_pool, "No thread pool existed in node finished callback for node {}", id);
                m_thread_pool->stop(id);
            });
        }
    }
}

void FramePipeline::reset()
{
    m_root.reset();
    m_thread_pool.reset();
}

void FramePipeline::process_thread(const std::string& node_id, FramePipelineDataType&& data)
{
    // auto it =
    //     std::ranges::find_if(m_threads, [&node_id](const auto& item) { return item.get_root_node_id() == node_id; });

    // STEP_ASSERT(it == m_threads.cend(), "No pipeline thread with node id {} existed in pipeline", node_id);

    // it->run(std::move(data));
}

}  // namespace step::video::pipeline