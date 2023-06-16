#pragma once

#include <core/base/interfaces/event_handler_list.hpp>
#include <core/exception/assert.hpp>

#include <core/threading/thread_pool_worker.hpp>

#include <proc/pipeline/pipeline_branch.hpp>

#include <list>

namespace step::proc {

template <typename TData>
class AsyncPipelineBranch
    : public step::threading::ThreadPoolWorker<PipelineIdType, PipelineDataPtr<TData>, PipelineDataPtr<TData>>,
      public PipelineBranch<TData>
{
    using ThreadPoolWorkerDataType = PipelineDataPtr<TData>;
    using ThreadPoolWorkerResultDataType = ThreadPoolWorkerDataType;
    using ThreadPoolWorkerType =
        step::threading::ThreadPoolWorker<PipelineIdType, PipelineDataPtr<TData>, PipelineDataPtr<TData>>;

public:
    AsyncPipelineBranch(const PipelineNodePtr<TData>& init_node)
        : PipelineBranch<TData>(init_node), ThreadPoolWorkerType(init_node->get_id())
    {
    }

    ~AsyncPipelineBranch() { ThreadPoolWorkerType::stop_worker(); }

    void add_node(const PipelineNodePtr<TData>& node) override
    {
        if (ThreadPoolWorkerType::is_running())
            return;

        PipelineBranch<TData>::add_node(node);
    }

private:
    ThreadPoolWorkerResultDataType thread_pool_worker_process_data(const ThreadPoolWorkerDataType& data) override
    {
        PipelineBranch<TData>::process(data);
        return data;
    }
};

}  // namespace step::proc