#pragma once

#include "pipeline_node.hpp"

#include <core/exception/assert.hpp>

#include <list>

namespace step::proc {

template <typename TData>
class PipelineBranch
{
public:
    PipelineBranch(const PipelineNodePtr<TData>& init_node = nullptr)
    {
        if (init_node)
            add_node(init_node);
    }

    virtual void add_node(const PipelineNodePtr<TData>& node) { m_list.push_back(node); }

    PipelineIdType get_id() const
    {
        STEP_ASSERT(!m_list.empty(), "Pipeline list is empty!");
        STEP_ASSERT(m_list.front(), "Pipeline list front is null!");
        return m_list.front()->get_id();
    }

    PipelineIdType get_last_id() const
    {
        STEP_ASSERT(!m_list.empty(), "Pipeline list is empty!");
        STEP_ASSERT(m_list.back(), "Pipeline list back is null!");
        return m_list.back()->get_id();
    }

    void process(const PipelineDataPtr<TData>& data)
    {
        for (const auto& node : m_list)
            node->process(data);
    }

private:
    std::list<PipelineNodePtr<TData>> m_list;
};

}  // namespace step::proc