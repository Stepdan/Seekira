#pragma once

#include <core/base/interfaces/event_handler_list.hpp>

#include <core/log/log.hpp>
#include <core/exception/assert.hpp>

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>

namespace step::threading {

template <typename IdType, typename TResultData>
class IThreadPoolWorkerEventObserver
{
public:
    virtual ~IThreadPoolWorkerEventObserver() = default;

    virtual void on_finished(const IdType& id, const TResultData& data) = 0;
};

template <typename IdType, typename TResultData>
class IThreadPoolWorkerEventSource
{
public:
    virtual ~IThreadPoolWorkerEventSource() = default;

    virtual void register_observer(IThreadPoolWorkerEventObserver<IdType, TResultData>* observer) = 0;
    virtual void unregister_observer(IThreadPoolWorkerEventObserver<IdType, TResultData>* observer) = 0;
};

template <typename IdType, typename TData, typename TResultData>
class ThreadPoolWorker : public IThreadPoolWorkerEventSource<IdType, TResultData>
{
    using DataType = TData;
    using ResultDataType = TResultData;

public:
    ThreadPoolWorker(IdType id) : m_id(id) {}
    ~ThreadPoolWorker()
    {
        STEP_LOG(L_TRACE, "ThreadPoolWorker {} destruction", m_id);
        stop_worker();
    }

    IdType get_id() const { return m_id; }

    bool is_running() const noexcept { return m_is_running; }

public:
    void add_data(TData&& data)
    {
        if (!m_is_running)
            run_worker();

        {
            std::scoped_lock lock(m_data_guard);
            m_data.emplace(std::move(data));
        }

        m_data_cnd.notify_one();
    }

    bool has_data() const
    {
        std::scoped_lock lock(m_data_guard);
        !m_data.empty();
    }

    void run_worker()
    {
        if (m_is_running)
            return;

        m_worker = std::thread(&ThreadPoolWorker::worker_thread, this);
        m_is_running.store(true);
        STEP_LOG(L_TRACE, "ThreadPoolWorker {} has been started", m_id);
    }

    void stop_worker()
    {
        if (!m_is_running)
            return;

        STEP_LOG(L_TRACE, "Stopping ThreadPoolWorker {}", m_id);
        m_need_stop.store(true);
        m_data_cnd.notify_one();

        if (m_worker.joinable())
            m_worker.join();

        reset_data();
        reset_exceptions();

        m_is_running.store(false);
        m_need_stop.store(false);

        STEP_LOG(L_TRACE, "ThreadPoolWorker {} has been stopped", m_id);
    }

public:
    bool has_exceptions() const
    {
        std::scoped_lock lock(m_exception_mutex);
        return !m_exception_ptrs.empty();
    }

    std::deque<std::exception_ptr> get_exceptions() const
    {
        std::scoped_lock lock(m_exception_mutex);
        return m_exception_ptrs;
    }

    void reset_exceptions()
    {
        size_t size = 0;
        {
            std::scoped_lock lock(m_exception_mutex);
            size = m_exception_ptrs.size();
        }

        {
            std::scoped_lock lock(m_exception_mutex);
            if (size == m_exception_ptrs.size())
            {
                m_exception_ptrs.clear();
            }
            else
            {
                std::deque<std::exception_ptr> new_exceptions;
                for (auto i = size; size < m_exception_ptrs.size(); ++i)
                    new_exceptions.push_back(m_exception_ptrs.at(i));
                m_exception_ptrs.swap(new_exceptions);
            }
        }
    }

protected:
    virtual ResultDataType thread_pool_worker_process_data(DataType&& data) = 0;

private:
    void reset_data()
    {
        std::scoped_lock lock(m_data_guard);
        auto empty = std::queue<DataType>();
        m_data.swap(empty);
    }

    void worker_thread()
    {
        while (!m_need_stop)
        {
            try
            {
                DataType data;
                {
                    std::unique_lock lock(m_data_guard);
                    m_data_cnd.wait(lock, [this]() { return !m_data.empty() || m_need_stop; });

                    if (m_need_stop)
                        continue;

                    if (m_data.empty())
                        STEP_THROW_RUNTIME("Empty data in ThreadPoolWorker");

                    data = m_data.front();
                    m_data.pop();
                }
                auto result_data = thread_pool_worker_process_data(std::move(data));

                // notify executor
                m_event_observers.perform_for_each_event_handler(
                    std::bind(&IThreadPoolWorkerEventObserver<IdType, ResultDataType>::on_finished,
                              std::placeholders::_1, m_id, result_data));
            }
            catch (...)
            {
                std::scoped_lock lock(m_exception_mutex);
                m_exception_ptrs.push_back(std::current_exception());
            }
        }
    }

    // IThreadPoolWorkerEventSource
public:
    void register_observer(IThreadPoolWorkerEventObserver<IdType, TResultData>* observer) override
    {
        m_event_observers.register_event_handler(observer);
    }

    void unregister_observer(IThreadPoolWorkerEventObserver<IdType, TResultData>* observer) override
    {
        m_event_observers.unregister_event_handler(observer);
    }

private:
    IdType m_id;

    std::atomic_bool m_is_running{false};
    std::atomic_bool m_need_stop{false};

    mutable std::mutex m_data_guard;
    std::condition_variable m_data_cnd;
    std::queue<DataType> m_data;

    std::thread m_worker;

    mutable std::mutex m_exception_mutex;
    std::deque<std::exception_ptr> m_exception_ptrs;

    EventHandlerList<IThreadPoolWorkerEventObserver<IdType, ResultDataType>> m_event_observers;
};

}  // namespace step::threading