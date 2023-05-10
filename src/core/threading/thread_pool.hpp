#pragma once

#include "thread_pool_worker.hpp"

#include <core/exception/assert.hpp>

#include <string>
#include <unordered_map>

namespace step::threading {

template <typename IdType, typename TData, typename TResultData>
class ThreadPool : public IThreadPoolWorkerEventObserver<IdType, TResultData>
{
protected:
    using DataType = TData;
    using ResultDataType = TResultData;
    using ThreadPoolWorkerType = ThreadPoolWorker<IdType, DataType, ResultDataType>;
    using ThreadPoolWorkerPtr = std::shared_ptr<ThreadPoolWorkerType>;

public:
    ThreadPool() = default;
    ~ThreadPool()
    {
        STEP_LOG(L_TRACE, "ThreadPool destruction");
        stop();
    }

    bool is_running() const { return m_is_running; }
    bool need_stop() const { return m_need_stop; }

    void add_thread_worker(const ThreadPoolWorkerPtr& tw_ptr)
    {
        STEP_ASSERT(tw_ptr, "Invalid ThreadPoolWorker ptr!");
        STEP_ASSERT(!m_threads.contains(tw_ptr->get_id()), "ThreadPool already contains ThreadPoolWorker {}",
                    tw_ptr->get_id());

        m_threads[tw_ptr->get_id()] = tw_ptr;
    }

    void run()
    {
        if (m_is_running)
        {
            STEP_LOG(L_WARN, "Can't run ThreadPool: is running now!");
            return;
        }

        STEP_LOG(L_TRACE, "ThreadPool starting");
        m_need_stop.store(false);
        m_worker = std::thread(&ThreadPool::worker_thread, this);
        m_is_running.store(true);
        STEP_LOG(L_TRACE, "ThreadPool has been started");
    }

    void stop()
    {
        if (!m_is_running)
            return;

        STEP_LOG(L_TRACE, "Stopping ThreadPool");
        m_need_stop.store(true);

        for (auto& [id, thread] : m_threads)
            thread->stop();

        thread_pool_stop_impl();

        if (m_worker.joinable())
            m_worker.join();

        m_is_running.store(false);
        m_need_stop.store(false);
        STEP_LOG(L_TRACE, "ThreadPool has been stopped");
    }

protected:
    virtual void thread_pool_stop_impl() = 0;
    virtual void thread_pool_iteration() = 0;

    virtual void on_finished(const IdType& id, ResultDataType data) override
    {
        STEP_UNDEFINED("on_finished is undefined!");
    }

private:
    void worker_thread()
    {
        while (!m_need_stop)
        {
            thread_pool_iteration();
        }
    }

protected:
    std::unordered_map<IdType, ThreadPoolWorkerPtr> m_threads;

private:
    std::atomic_bool m_is_running{false};
    std::atomic_bool m_need_stop{false};

    std::mutex m_stop_guard;

    std::thread m_worker;
};

}  // namespace step::threading