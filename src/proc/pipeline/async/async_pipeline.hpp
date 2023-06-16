#pragma once

#include "async_pipeline_branch.hpp"

#include <core/threading/thread_pool_execute_policy.hpp>
#include <core/threading/thread_pool.hpp>
#include <core/log/log.hpp>

#include <proc/pipeline/pipeline.hpp>

namespace step::proc {

template <typename TData>
using PipelineResultMap = robin_hood::unordered_map<PipelineIdType, TData>;

template <typename TData>
class IPipelineEventObserver
{
public:
    virtual ~IPipelineEventObserver() = default;

    virtual void on_pipeline_data_update(const PipelineResultMap<TData>& data) = 0;
};

template <typename TData>
class IPipelineEventSource
{
public:
    virtual ~IPipelineEventSource() = default;

    virtual void register_observer(IPipelineEventObserver<TData>* observer) = 0;
    virtual void unregister_observer(IPipelineEventObserver<TData>* observer) = 0;
};

template <typename TData>
class AsyncPipeline : public BasePipeline<TData>,
                      public threading::ThreadPool<PipelineIdType, PipelineDataPtr<TData>, PipelineDataPtr<TData>>,
                      public IPipelineEventSource<std::shared_ptr<PipelineData<TData>>>
{
protected:
    using ThreadPoolDataType = PipelineDataPtr<TData>;
    using ThreadPoolResultDataType = ThreadPoolDataType;
    using ThreadPoolType = step::threading::ThreadPool<PipelineIdType, PipelineDataPtr<TData>, PipelineDataPtr<TData>>;

    using OutputDataMapType = robin_hood::unordered_map<PipelineIdType, PipelineDataPtr<TData>>;

public:
    AsyncPipeline(const ObjectPtrJSON& config) : BasePipeline<TData>(config) {}

    virtual ~AsyncPipeline()
    {
        STEP_LOG(L_TRACE, "AsyncPipeline {} destruction", BasePipeline<TData>::m_settings.name);
        ThreadPoolType::stop();
        // Stop (thread_pool_stop_impl) will be called from ThreadPool::stop during ThreadPool destruction
    }

    bool add_process_data(PipelineDataPtr<TData>&& data)
    {
        if (!ThreadPoolType::is_running())
            ThreadPoolType::run();

        return add_process_data(BasePipeline<TData>::get_root_id(), std::move(data));
    }

private:
    bool add_process_data(const PipelineIdType& id, PipelineDataPtr<TData>&& data)
    {
        std::scoped_lock lock(m_branches_data_guard);
        auto& branch_data = m_branches_data[id];
        if (branch_data.status != BranchStatus::Finished)
            return false;

        branch_data = {std::move(data), BranchStatus::Ready};
        return true;
    }

    void thread_pool_stop_impl() override
    {
        // NO LOCK THERE
        // Under thread_pool mutex m_stop_guard locking.
        // All threads already joined and stopped
        STEP_LOG(L_INFO, "Stopping pipeline {}", BasePipeline<TData>::m_settings.name);

        for (auto& [id, branch_data] : m_branches_data)
            branch_data = {nullptr, BranchStatus::Finished};

        STEP_LOG(L_INFO, "Pipeline {} has been stopped", BasePipeline<TData>::m_settings.name);
    }

    void thread_pool_iteration() override
    {
        const auto& root_id = BasePipeline<TData>::get_root_id();
        bool is_ready;

        std::scoped_lock lock(m_branches_data_guard);

        // Check for exceptions
        for (auto& [id, branch] : ThreadPoolType::m_threads)
        {
            if (!branch->has_exceptions())
                continue;

            STEP_LOG(L_ERROR, "Pipeline {}: Handle exceptions from branch {}", BasePipeline<TData>::m_settings.name,
                     id);
            for (auto branch_exception : branch->get_exceptions())
            {
                try
                {
                    std::rethrow_exception(branch_exception);
                }
                catch (const std::exception& e)
                {
                    STEP_LOG(L_ERROR, "Pipeline {}: Branch {} exception: {}", BasePipeline<TData>::m_settings.name, id,
                             e.what());
                    m_branches_data[id] = {nullptr, BranchStatus::Finished};
                    m_output_branches_data[id].reset();
                }
            }
            // TODO exception handling
            branch->reset_exceptions();
        }

        // For ParallelWait we ensure that all branches are stopped and input are ready
        bool ready_for_wait_iteration = true;
        if (BasePipeline<TData>::m_settings.sync_policy == PipelineSyncPolicy::ParallelWait)
        {
            ready_for_wait_iteration = std::ranges::all_of(m_branches_data, [&root_id](const auto& item) {
                return item.first == root_id ? item.second.status == BranchStatus::Ready
                                             : item.second.status == BranchStatus::Finished;
            });
        }

        for (auto& [id, branch_data] : m_branches_data)
        {
            // Skip if branch is running now
            if (branch_data.status == BranchStatus::Running)
                continue;

            is_ready = branch_data.data_to_process && branch_data.status == BranchStatus::Ready;

            if (BasePipeline<TData>::m_settings.sync_policy == PipelineSyncPolicy::ParallelWait)
            {
                // For ParallelWait we should ensure that we are processing input with all stopped other branches
                is_ready = is_ready && (root_id != id || root_id == id && ready_for_wait_iteration);
            }

            // Skip if smth is not ready
            if (!is_ready)
                continue;

            STEP_ASSERT(branch_data.data_to_process && branch_data.status == BranchStatus::Ready,
                        "Invalid pipeline process data for start!");

            STEP_LOG(L_INFO, "Pipeline {} start branch {}", BasePipeline<TData>::m_settings.name, id);

            // Copy processing data and clear it from storage
            //    to skip next data before finishing

            ThreadPoolType::m_threads[id]->add_data(std::move(branch_data.data_to_process));
            branch_data = {nullptr, BranchStatus::Running};
            m_output_branches_data[id].reset();
        }
    }

private:
    void create_branch(const PipelineNodePtr<TData>& branch_root) override
    {
        auto id = branch_root->get_id();
        ThreadPoolType::add_thread_worker(std::make_shared<AsyncPipelineBranch<TData>>(branch_root));

        auto& branch = ThreadPoolType::m_threads[id];
        branch->register_observer(this);
        m_branches_data[id] = {nullptr, BranchStatus::Finished};
    }

    void add_node_to_branch(const PipelineIdType& branch_id, const PipelineNodePtr<TData>& node) override
    {
        STEP_ASSERT(node, "Can't add node to branch {}: empty node", branch_id);
        STEP_ASSERT(ThreadPoolType::m_threads.contains(branch_id), "AsyncPipeline doesn't have branch {}", branch_id);

        auto branch = std::dynamic_pointer_cast<PipelineBranch<TData>>(ThreadPoolType::m_threads[branch_id]);
        branch->add_node(node);
    }

    // IThreadPoolWorkerEventObserver
private:
    void on_finished(const PipelineIdType& id, const ThreadPoolResultDataType& data) override
    {
        STEP_LOG(L_INFO, "on_finished branch {}", id);
        if (ThreadPoolType::need_stop())
        {
            STEP_LOG(L_INFO, "Skip on_finished branch {} due stopping", id);
            return;
        }

        const auto branch_last_id =
            std::dynamic_pointer_cast<PipelineBranch<TData>>(ThreadPoolType::m_threads[id])->get_last_id();

        const auto& children_ids = BasePipeline<TData>::get_node(branch_last_id)->get_children_ids();

        std::scoped_lock lock(m_branches_data_guard);
        m_branches_data[id] = {nullptr, BranchStatus::Finished};
        for (const auto& child_id : children_ids)
        {
            auto& branch_data = m_branches_data[child_id];
            if (branch_data.status == BranchStatus::Finished)
                branch_data = {clone_pipeline_data(data), BranchStatus::Ready};
        }

        m_output_branches_data[id] = clone_pipeline_data(data);
        // Отправляем данные подписчикам
        decltype(m_output_branches_data) output;
        if (BasePipeline<TData>::m_settings.sync_policy == PipelineSyncPolicy::ParallelNoWait)
        {
            output[id] = clone_pipeline_data(data);
        }
        else if (BasePipeline<TData>::m_settings.sync_policy == PipelineSyncPolicy::ParallelWait)
        {
            // В режиме ожидания надо убедиться, что у нас полностью заполнена мапа с результатами
            bool is_all_ready =
                std::all_of(ThreadPoolType::m_threads.cbegin(), ThreadPoolType::m_threads.cend(),
                            [this](const std::pair<PipelineIdType, ThreadPoolType::ThreadPoolWorkerPtr>& item) {
                                return m_output_branches_data.contains(item.first);
                            });

            if (is_all_ready)
            {
                for (const auto& [output_id, output_data] : m_output_branches_data)
                    output[output_id] = clone_pipeline_data(output_data);
            }
        }

        m_pipeline_observers.perform_for_each_event_handler(
            std::bind(&IPipelineEventObserver<std::shared_ptr<PipelineData<TData>>>::on_pipeline_data_update,
                      std::placeholders::_1, output));
    }

    // IPipelineEventSource<std::shared_ptr<PipelineData<TData>>>
public:
    void register_observer(IPipelineEventObserver<std::shared_ptr<PipelineData<TData>>>* observer) override
    {
        m_pipeline_observers.register_event_handler(observer);
    }

    void unregister_observer(IPipelineEventObserver<std::shared_ptr<PipelineData<TData>>>* observer) override
    {
        m_pipeline_observers.unregister_event_handler(observer);
    }

private:
    /*
        For thread pool iterations
    */
    enum class BranchStatus
    {
        /* clang-format off */
        Ready       = 1, // Is stopped and has data for processing
        Running     = 2, // Is running, data is empty
        //NeedStop    = 3, // Finished but stop requires, data is empty
        Finished    = 4, // Is stopped, no data for processing
        /* clang-format on */
    };

    template <typename TProcessData>
    struct BranchProcessData
    {
        TProcessData data_to_process{nullptr};
        BranchStatus status{BranchStatus::Finished};
    };
    mutable std::mutex m_branches_data_guard;
    // Здесь входные данные для каждой ветви пайплайна
    robin_hood::unordered_map<PipelineIdType, BranchProcessData<PipelineDataPtr<TData>>> m_branches_data;

    /*
        Здесь выходные данные каждой ветви пайплайна.
        При синхронном режиме - отправляем полностью всю мапу, когда все ветви пайплайна отработали
        При режиме без ожидания - отправляем данные только той ветви, которая закончила работу
    */
    PipelineResultMap<PipelineDataPtr<TData>> m_output_branches_data;
    EventHandlerList<IPipelineEventObserver<PipelineDataPtr<TData>>, threading::ThreadPoolExecutePolicy<0>>
        m_pipeline_observers;
};

}  // namespace step::proc