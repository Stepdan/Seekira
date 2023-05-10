#pragma once

#include "pipeline_node.hpp"

#include <core/base/interfaces/event_handler_list.hpp>
#include <core/exception/assert.hpp>

#include <core/threading/thread_pool_worker.hpp>

#include <list>

namespace step::pipeline {

template <typename TData>
class PipelineBranch
    : public step::threading::ThreadPoolWorker<IdType, PipelineDataTypePtr<TData>, PipelineDataTypePtr<TData>>
{
    using ThreadPoolWorkerDataType = PipelineDataTypePtr<TData>;
    using ThreadPoolWorkerResultDataType = ThreadPoolWorkerDataType;
    using ThreadPoolWorkerType =
        step::threading::ThreadPoolWorker<IdType, PipelineDataTypePtr<TData>, PipelineDataTypePtr<TData>>;

public:
    PipelineBranch(std::shared_ptr<PipelineGraphNode<TData>> init_node) : ThreadPoolWorkerType(init_node->get_id())
    {
        add_node(init_node);
    }

    ~PipelineBranch() { ThreadPoolWorkerType::stop(); }

    void add_node(std::shared_ptr<PipelineGraphNode<TData>> node)
    {
        if (ThreadPoolWorkerType::is_running())
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
    ThreadPoolWorkerResultDataType thread_pool_worker_process_data(ThreadPoolWorkerDataType&& data) override
    {
        ThreadPoolWorkerResultDataType process_data = std::move(data);
        std::ranges::for_each(m_list, [&process_data](const std::shared_ptr<PipelineGraphNode<TData>>& node) {
            node->process(process_data);
        });

        return process_data;
    }

private:
    std::list<std::shared_ptr<PipelineGraphNode<TData>>> m_list;
};

}  // namespace step::pipeline