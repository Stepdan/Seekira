#pragma once

#include "pipeline_branch.hpp"
#include "pipeline_settings.hpp"

#include <core/graph/graph.hpp>

#include <set>

namespace step::proc {

template <typename TData>
class BasePipeline : public graph::BaseGraph, public ISerializable
{
public:
    BasePipeline() {}

    virtual ~BasePipeline() { STEP_LOG(L_TRACE, "BasePipeline {} destruction", m_settings.name); }

    void initialize(const ObjectPtrJSON& pipeline_json)
    {
        graph::GraphSettings graph_settings;
        graph_settings.one_parent = true;
        set_settings(graph_settings);

        deserialize(pipeline_json);
    }

    PipelineIdType get_root_id() const
    {
        STEP_ASSERT(m_root, "Pipeline {} can'r provide root id: empty root!", m_settings.name);
        return m_root->get_id();
    }

    PipelineSyncPolicy get_sync_policy() const { return m_settings.sync_policy; }

    virtual void process(const PipelineDataPtr<TData>& data) { STEP_UNDEFINED("BasePipeline process is undefined!"); }

protected:
    virtual void create_branch(const PipelineNodePtr<TData>& branch_root) = 0;
    virtual void add_node_to_branch(const PipelineIdType& branch_id, const PipelineNodePtr<TData>& node) = 0;

    // Data parsing
protected:
    void deserialize(const ObjectPtrJSON& pipeline_json)
    {
        STEP_LOG(L_INFO, "Start pipeline deserializing");
        m_root.reset();

        auto settings_json = json::get_object(pipeline_json, CFG_FLD::SETTINGS);
        m_settings = PipelineSettings(settings_json);
        const auto& pipeline_name = m_settings.name;
        STEP_LOG(L_INFO, "Pipeline name: {}", pipeline_name);

        auto nodes_collection = json::get_array(pipeline_json, CFG_FLD::NODES);
        json::for_each_in_array<ObjectPtrJSON>(nodes_collection, [this](const ObjectPtrJSON& node_cfg) {
            auto node = std::make_shared<PipelineNode<TData>>(node_cfg);
            add_node(node->get_id(), node);
        });

        auto links_collection = json::get_array(pipeline_json, CFG_FLD::LINKS);
        json::for_each_in_array<ArrayPtrJSON>(links_collection, [this, &pipeline_name](const ArrayPtrJSON& link_cfg) {
            STEP_ASSERT(link_cfg->size() == 2, "Pipeline {}: Invalid link", pipeline_name);
            std::vector<std::string> ids;
            json::for_each_in_array<std::string>(link_cfg, [&ids](const std::string& id) { ids.push_back(id); });

            add_edge(ids.front(), ids.back());
        });

        m_root = std::dynamic_pointer_cast<PipelineNode<TData>>(get_node("input_node"));

        init_branches();
    }

    void init_branches()
    {
        std::set<PipelineIdType> branch_ids;
        for (const auto& id : get_nodes_without_parents())
            branch_ids.insert(id);

        for (const auto& parent_id : get_nodes_with_children())
            for (const auto& child_id : get_node(parent_id)->get_children_ids())
                branch_ids.insert(child_id);

        for (const auto& id : branch_ids)
        {
            auto list_front_id = id;
            auto processed_id = id;
            auto node = get_node(processed_id);

            auto pipeline_node = std::dynamic_pointer_cast<PipelineNode<TData>>(node);
            create_branch(pipeline_node);

            if (node->get_children_ids().empty())
                continue;

            processed_id = node->get_children_ids().front();

            while (!branch_ids.contains(processed_id))
            {
                auto node = get_node(processed_id);
                add_node_to_branch(list_front_id, std::dynamic_pointer_cast<PipelineNode<TData>>(node));

                if (node->get_children_ids().empty())
                    break;

                processed_id = node->get_children_ids().front();
            }
        }
    }

protected:
    PipelineSettings m_settings;
    PipelineNodePtr<TData> m_root;
};

}  // namespace step::proc