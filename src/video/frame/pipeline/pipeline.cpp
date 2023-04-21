#include "pipeline.hpp"

#include <video/frame/pipeline/nodes/input_node.hpp>

#include <base/utils/exception/assert.hpp>

namespace {

constexpr std::string_view INPUT_NODE_ID = "input_node";

constexpr std::string_view SINKS_FIELD = "sinks";

}  // namespace

namespace step::video::pipeline {

FramePipeline::FramePipeline(const ObjectPtrJSON& config) { deserialize(config); }

void FramePipeline::deserialize(const ObjectPtrJSON& config)
{
    init_with_input_node();

    auto pipeline_collection = json::get_array(config, PIPELINE_FIELD.data());
    STEP_ASSERT(pipeline_collection && !pipeline_collection->empty(), "Pipeline config is empty!");

    std::map<std::string, std::pair<FramePipelineGraphNode::Ptr, ArrayPtrJSON>> unprocessed_elements;
    std::vector<std::string> processed_node_ids;
    unprocessed_elements[INPUT_NODE_ID.data()] = {m_root, pipeline_collection};

    while (!unprocessed_elements.empty())
    {
        auto [node_ptr, collection] = unprocessed_elements.begin()->second;

        const auto& node_id = node_ptr->get_id();

        /* clang-format off */
        json::for_each_in_array<ObjectPtrJSON>(collection,
        [this, &node_id, &node_ptr, &unprocessed_elements, &processed_node_ids](ObjectPtrJSON& node_cfg)
        {
            auto sink_ptr = std::make_shared<FramePipelineGraphNode>(node_cfg, [this](const std::string& node_id, FramePipelineDataType&& data)
            {
                process_thread(node_id, std::move(data));
            });
            node_ptr->add_sink(sink_ptr);

            auto sink_collection = json::opt_array(node_cfg, SINKS_FIELD.data());
            
            STEP_ASSERT(sink_ptr->get_id() != node_id && std::ranges::none_of(processed_node_ids, [&sink_ptr](const auto& item){ return sink_ptr->get_id() == item; }),
                        "Node with id {} already processed", sink_ptr->get_id());
        });

        processed_node_ids.push_back(node_id);
        unprocessed_elements.extract(node_id);

        /* clang-format on */
    }
}

void FramePipeline::init_with_input_node()
{
    /* clang-format off */
    m_root = std::make_shared<FramePipelineGraphNode>(INPUT_NODE_ID.data(), std::make_unique<InputPipelineNode>(),
        [this](const std::string& node_id, FramePipelineDataType&& data) { process_thread(node_id, std::move(data)); });
    /* clang-format on */
}

void FramePipeline::process_thread(const std::string& node_id, FramePipelineDataType&& data)
{
    if (m_main_thread.get_root_node_id() == node_id)
        m_main_thread.run(std::move(data));

    auto it = std::ranges::find_if(m_branch_threads,
                                   [&node_id](const auto& item) { return item.get_root_node_id() == node_id; });

    STEP_ASSERT(it == m_branch_threads.cend(), "No pipeline thread with node id {} existed in pipeline", node_id);

    it->run(std::move(data));
}

}  // namespace step::video::pipeline