#include "pipeline_thread_pool.hpp"

#include <base/utils/exception/assert.hpp>

#include <log/log.hpp>

namespace step::video::pipeline {

PipelineThreadPool::PipelineThreadPool() {}

PipelineThreadPool::~PipelineThreadPool() { reset(); }

void PipelineThreadPool::set_pipeline_settings(const std::shared_ptr<FramePipelineSettings>& settings)
{
    std::scoped_lock lock(m_guard);
    m_pipeline_settings = settings;
}

void PipelineThreadPool::add_thread(std::unique_ptr<PipelineThread>&& thread)
{
    std::scoped_lock lock(m_guard);
    STEP_ASSERT(thread, "Pipeline thread pool: try to add empty thread!");

    const auto thread_id = thread->get_root_node_id();
    STEP_ASSERT(m_threads.contains(thread_id), "Pipeline thread pool: thread with id {} already existed", thread_id);

    m_threads.emplace(thread_id, std::move(thread));
}

void PipelineThreadPool::reset()
{
    std::scoped_lock lock(m_guard);
    for (auto& [id, thread] : m_threads)
    {
        if (thread->joinable())
            thread->join();
    }
}

void PipelineThreadPool::stop(const std::string& thread_id)
{
    std::scoped_lock lock(m_guard);

    STEP_ASSERT(m_threads.contains(thread_id), "Can't stop thread {}: pipeline thread pool doesn't contain thread!",
                thread_id);

    auto& thread = m_threads[thread_id];
    if (thread->joinable())
        thread->join();
}

void PipelineThreadPool::run(const std::string& thread_id, FramePipelineDataType&& data)
{
    std::scoped_lock lock(m_guard);
    auto settings = m_pipeline_settings.lock();
    STEP_ASSERT(settings, "Invalid pipeline settings in PipelineThreadPool");
    STEP_ASSERT(m_threads.contains(thread_id), "Pipeline thread pool doesn't contain thread with id {}", thread_id);

    /*
        Thread launching depends on pipeline sync policy
    */
    switch (settings->get_sync_policy())
    {
        case FramePipelineSyncPolicy::Sync:
            run_sync(thread_id, std::move(data));
        case FramePipelineSyncPolicy::ParallelWait:
            run_parallel_wait(thread_id, std::move(data));
        case FramePipelineSyncPolicy::ParallelNoWait:
            run_parallel_no_wait(thread_id, std::move(data));
        default:
            STEP_UNDEFINED("Undefined behaviour!");
    }
}

void PipelineThreadPool::run_sync(const std::string& thread_id, FramePipelineDataType&& data)
{
    STEP_UNDEFINED("PipelineThreadPool::run_sync is undefined!");
}

void PipelineThreadPool::run_parallel_wait(const std::string& thread_id, FramePipelineDataType&& data)
{
    /*
        Wait for all threads finishing.
        Skip data if we have at least one working thread.
    */
    auto& thread = m_threads[thread_id];

    if (thread_id == m_main_thread_id)
    {
        bool is_all_stopped = true;
        for (auto& [id, thread] : m_threads)
            is_all_stopped = is_all_stopped && thread->is_running();

        if (is_all_stopped)
            thread->run(std::move(data));
        else
            STEP_LOG(L_TRACE, "Pipeline skip data for wait policy");
    }
    else
    {
        if (thread->is_running())
        {
            STEP_LOG(L_WARN, "Thread {} is running, finished expected", thread_id);
            if (thread->joinable())
                thread->join();
        }

        thread->run(std::move(data));
    }
}

void PipelineThreadPool::run_parallel_no_wait(const std::string& thread_id, FramePipelineDataType&& data)
{
    auto& thread = m_threads[thread_id];

    /*
        Skip data because of PipelineNoWait policy
    */
    if (thread->is_running())
        return;

    thread->run(std::move(data));
}

}  // namespace step::video::pipeline