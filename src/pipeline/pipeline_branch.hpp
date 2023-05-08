#pragma once

#include "pipeline_node.hpp"

#include <base/interfaces/event_handler_list.hpp>
#include <base/utils/exception/assert.hpp>

#include <base/utils/thread/thread_worker.hpp>

#include <list>

namespace step::pipeline {

template <typename TData>
class IPipelineBranchEventObserver
{
public:
    virtual ~IPipelineBranchEventObserver() = default;

    virtual void on_branch_finished(std::pair<IdType, IdType> branch_ids, PipelineDataTypePtr<TData> data) = 0;
};

template <typename TData>
class IPipelineBranchEventSource
{
public:
    virtual ~IPipelineBranchEventSource() = default;

    virtual void register_observer(IPipelineBranchEventObserver<TData>* observer) = 0;
    virtual void unregister_observer(IPipelineBranchEventObserver<TData>* observer) = 0;
};

template <typename TData>
class PipelineBranch : public step::utils::ThreadWorker<PipelineDataTypePtr<TData>>,
                       public IPipelineBranchEventSource<TData>
{
    using ThreadWorkerDataType = PipelineDataTypePtr<TData>;
    using ThreadWorkerType = step::utils::ThreadWorker<ThreadWorkerDataType>;

public:
    ~PipelineBranch() { ThreadWorkerType::stop(); }

    std::exception_ptr get_exception() const { return m_exception_ptr; }
    void reset_exception() { m_exception_ptr = nullptr; }

    void add_node(std::shared_ptr<PipelineGraphNode<TData>> node)
    {
        if (ThreadWorkerType::is_running())
            return;

        std::scoped_lock lock(m_list_guard);
        m_list.push_back(node);
    }

    std::pair<IdType, IdType> get_ids() const
    {
        std::scoped_lock lock(m_list_guard);
        STEP_ASSERT(!m_list.empty(), "Pipeline list is empty!");
        STEP_ASSERT(m_list.front(), "Pipeline list front is null!");
        STEP_ASSERT(m_list.back(), "Pipeline list back is null!");

        return {m_list.front()->get_id(), m_list.back()->get_id()};
    }

    virtual void stop() override
    {
        STEP_LOG(L_INFO, "Stopping branch {}", get_ids().first);
        ThreadWorkerType::stop();
        STEP_LOG(L_INFO, "Branch {} has been stopped", get_ids().first);
    }

private:
    void process_data(ThreadWorkerDataType&& data) override
    {
        auto process_data = std::move(data);
        std::ranges::for_each(m_list, [&process_data](const std::shared_ptr<PipelineGraphNode<TData>>& node) {
            node->process(process_data);
        });
        // notify executor
        m_event_observers.perform_for_each_event_handler(
            std::bind(&IPipelineBranchEventObserver<TData>::on_branch_finished, std::placeholders::_1, get_ids(),
                      clone_pipeline_data_shared(process_data)));
    }

    // IPipelineListEventSource
public:
    void register_observer(IPipelineBranchEventObserver<TData>* observer) override
    {
        m_event_observers.register_event_handler(observer);
    }

    void unregister_observer(IPipelineBranchEventObserver<TData>* observer) override
    {
        m_event_observers.unregister_event_handler(observer);
    }

private:
    mutable std::mutex m_list_guard;
    std::list<std::shared_ptr<PipelineGraphNode<TData>>> m_list;
    EventHandlerList<IPipelineBranchEventObserver<TData>> m_event_observers;

    // Exception handling
    std::exception_ptr m_exception_ptr{nullptr};
};

}  // namespace step::pipeline