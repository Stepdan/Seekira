#pragma once

#include "pipeline_node.hpp"

#include <base/interfaces/event_handler_list.hpp>
#include <base/utils/exception/assert.hpp>

#include <atomic>
#include <list>
#include <mutex>
#include <thread>

namespace step::pipeline {

template <typename TData>
class IPipelineBranchEventObserver
{
public:
    virtual ~IPipelineBranchEventObserver() = default;

    virtual void on_branch_finished(std::pair<IdType, IdType> branch_ids,
                                    std::shared_ptr<PipelineData<TData>> data) = 0;
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
class PipelineBranch : public IPipelineBranchEventSource<TData>
{
public:
    ~PipelineBranch() { stop(); }

    std::exception_ptr get_exception() const { return m_exception_ptr; }
    void reset_exception() { m_exception_ptr = nullptr; }

    void add(std::shared_ptr<PipelineGraphNode<TData>> node)
    {
        if (m_is_running)
            return;

        std::scoped_lock lock(m_guard);
        m_list.push_back(node);
    }

    std::pair<IdType, IdType> get_ids() const
    {
        STEP_ASSERT(!m_list.empty(), "Pipeline list is empty!");
        STEP_ASSERT(m_list.front(), "Pipeline list front is null!");
        STEP_ASSERT(m_list.back(), "Pipeline list back is null!");

        return {m_list.front()->get_id(), m_list.back()->get_id()};
    }

    void run(const std::shared_ptr<PipelineData<TData>>& data)
    {
        if (m_exception_ptr)
        {
            STEP_LOG(L_WARN, "Can't run branch {}: unhandling exception, stop required.", get_ids().first);
            return;
        }

        if (m_is_running)
            return;

        std::scoped_lock lock(m_guard);

        m_worker = std::thread([this, processed_data = clone_pipeline_data_shared(data)]() {
            try
            {
                std::ranges::for_each(m_list, [&processed_data](const std::shared_ptr<PipelineGraphNode<TData>>& node) {
                    node->process(processed_data);
                });
                // notify executor
                m_event_observers.perform_for_each_event_handler(
                    std::bind(&IPipelineBranchEventObserver<TData>::on_branch_finished, std::placeholders::_1,
                              get_ids(), processed_data));
            }
            catch (...)
            {
                STEP_LOG(L_WARN, "Catch exception in branch {}", get_ids().first);
                m_exception_ptr = std::current_exception();
            }
        });

        m_is_running = true;
    }

    void stop()
    {
        if (m_worker.joinable())
            m_worker.join();

        m_is_running = false;
        m_exception_ptr = nullptr;
    }

    bool is_running() const noexcept { return m_is_running; }

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
    std::atomic_bool m_is_running;
    std::mutex m_guard;
    std::thread m_worker;

    std::list<std::shared_ptr<PipelineGraphNode<TData>>> m_list;
    EventHandlerList<IPipelineBranchEventObserver<TData>> m_event_observers;

    // Exception handling
    std::exception_ptr m_exception_ptr{nullptr};
};

}  // namespace step::pipeline