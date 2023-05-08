#pragma once

#include "pipeline_node.hpp"

#include <base/interfaces/event_handler_list.hpp>
#include <base/utils/exception/assert.hpp>

#include <base/utils/thread/thread_worker.hpp>

#include <list>

namespace step::pipeline {

template <typename TData>
class PipelineBranch : public step::utils::ThreadWorker<IdType, PipelineDataTypePtr<TData>, PipelineDataTypePtr<TData>>
{
    using ThreadWorkerDataType = PipelineDataTypePtr<TData>;
    using ThreadWorkerResultDataType = ThreadWorkerDataType;
    using ThreadWorkerType = step::utils::ThreadWorker<IdType, PipelineDataTypePtr<TData>, PipelineDataTypePtr<TData>>;

public:
    PipelineBranch(std::shared_ptr<PipelineGraphNode<TData>> init_node) : ThreadWorkerType(init_node->get_id())
    {
        add_node(init_node);
    }

    ~PipelineBranch() { ThreadWorkerType::stop(); }

    void add_node(std::shared_ptr<PipelineGraphNode<TData>> node)
    {
        if (ThreadWorkerType::is_running())
            return;

        m_list.push_back(node);
    }

    IdType get_id() const
    {
        STEP_ASSERT(!m_list.empty(), "Pipeline list is empty!");
        STEP_ASSERT(m_list.front(), "Pipeline list front is null!");
        return m_list.front()->get_id();
    }

    IdType get_last_id() const
    {
        STEP_ASSERT(!m_list.empty(), "Pipeline list is empty!");
        STEP_ASSERT(m_list.back(), "Pipeline list back is null!");
        return m_list.back()->get_id();
    }

private:
    ThreadWorkerResultDataType process_data(ThreadWorkerDataType&& data) override
    {
        ThreadWorkerResultDataType process_data = std::move(data);
        std::ranges::for_each(m_list, [&process_data](const std::shared_ptr<PipelineGraphNode<TData>>& node) {
            node->process(process_data);
        });

        return process_data;
    }

private:
    std::list<std::shared_ptr<PipelineGraphNode<TData>>> m_list;
};

}  // namespace step::pipeline