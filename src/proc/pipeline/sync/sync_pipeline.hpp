#pragma once

#include <proc/pipeline/pipeline.hpp>

#include <queue>

namespace step::proc {

template <typename TData>
class SyncPipeline : public BasePipeline<TData>
{
public:
    SyncPipeline(const ObjectPtrJSON& config) : BasePipeline<TData>(config) {}

    void process(const PipelineDataPtr<TData>& data) override
    {
        std::queue<PipelineIdType> id_queue;
        id_queue.push(BasePipeline<TData>::get_root_id());
        STEP_LOG(L_DEBUG, "Start SyncPipeline {} process", BasePipeline<TData>::m_settings.name);

        while (!id_queue.empty())
        {
            const auto id = id_queue.front();
            STEP_LOG(L_DEBUG, "Process node {}", id);
            auto& branch = m_branches[id];
            branch.process(data);
            id_queue.pop();

            for (const auto& child_id : BasePipeline<TData>::get_node(branch.get_last_id())->get_children_ids())
                id_queue.push(child_id);
        }
    }

private:
    void create_branch(const PipelineNodePtr<TData>& branch_root) override
    {
        STEP_ASSERT(branch_root, "Can't create branch: empty root");
        STEP_ASSERT(!m_branches.contains(branch_root->get_id()), "SyncPipeline already has branch {}",
                    branch_root->get_id());
        m_branches[branch_root->get_id()] = PipelineBranch<TData>(branch_root);
    }

    void add_node_to_branch(const PipelineIdType& branch_id, const PipelineNodePtr<TData>& node) override
    {
        STEP_ASSERT(node, "Can't add node to branch {}: empty node", branch_id);
        STEP_ASSERT(m_branches.contains(branch_id), "SyncPipeline doesn't have branch {}", branch_id);
        m_branches[branch_id].add_node(node);
    }

private:
    std::map<PipelineIdType, PipelineBranch<TData>> m_branches;
};

}  // namespace step::proc