#pragma once

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>

#include <log/log.hpp>

namespace step::utils {

template <typename TData>
class ThreadWorker
{
    using DataType = TData;

public:
    ThreadWorker() = default;
    ~ThreadWorker() { stop(); }

    bool is_running() const noexcept { return m_is_running; }

public:
    void add_data(TData&& data)
    {
        if (!m_is_running)
            run();

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

    void run()
    {
        if (m_is_running)
            return;

        m_worker = std::thread(&ThreadWorker::worker_thread, this);
        m_is_running.store(true);
    }

    virtual void stop()
    {
        STEP_LOG(L_TRACE, "Stopping ThreadWorker");
        m_need_stop.store(true);
        m_data_cnd.notify_one();

        if (m_worker.joinable())
            m_worker.join();

        reset_data();
        reset_exceptions();

        m_is_running.store(false);

        STEP_LOG(L_TRACE, "ThreadWorker has been stopped");
    }

public:
    bool has_exceptions() const
    {
        std::scoped_lock lock(m_exception_mutex);
        return !m_exception_ptrs.empty();
    }

    void add_exception(std::exception_ptr ex)
    {
        std::scoped_lock lock(m_exception_mutex);
        return m_exception_ptrs.push_back(ex);
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
    virtual void process_data(DataType&& data) = 0;

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
            std::unique_lock lock(m_data_guard);
            m_data_cnd.wait(lock, [this]() { return !m_data.empty() || m_need_stop; });

            if (m_need_stop)
                continue;

            if (!m_data.empty())
            {
                try
                {
                    auto data = m_data.front();
                    m_data.pop();
                    process_data(std::move(data));
                }
                catch (...)
                {
                    std::scoped_lock lock(m_exception_mutex);
                    m_exception_ptrs.push_back(std::current_exception());
                }
            }
        }
    }

private:
    std::atomic_bool m_is_running{false};
    std::atomic_bool m_need_stop{false};

    mutable std::mutex m_data_guard;
    std::condition_variable m_data_cnd;
    std::queue<DataType> m_data;

    std::thread m_worker;

    mutable std::mutex m_exception_mutex;
    std::deque<std::exception_ptr> m_exception_ptrs;
};

}  // namespace step::utils