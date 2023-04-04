#pragma once

#include <core/types/time.hpp>

#include <condition_variable>
#include <mutex>

namespace step::utils {

/*! @brief A threading primitive that helps to maintain sequential execution
           flow in multi-threaded contexts based on the std::condition_variable
           semantics.
*/
struct ExecutionBarrier
{
public:
    /*! @brief Blocks the current thread until woken up or a timeout is reached.

        Waits in the current thread until woken up by a notify(), or until a
        timeout is reached. Returns whether the wake up happened due to the
        expired timeout.

        @sa notify()
    */
    std::cv_status wait_for(Milliseconds timeout)
    {
        constexpr auto noop = []() {};
        return wait_for(noop, timeout);
    }

    /*! @brief Blocks the current thread until woken up or a timeout is reached,
               performing a user-specified operation right before blocking.

        Performs a user-provided operation @a op, then blocks the current thread
        until woken up by a notify(), or until a timeout is reached. Returns
        whether the wake up happened due to the expired timeout.

        @note The user-provided operation and further thread block are performed
        as if atomically. This guarantees that, if a user operation indirectly
        causes a call to this barrier's notify(), the waiting thread would
        receive that notify call even if the notify happened before wait.

        @note The @a timeout specified doesn't include the time it takes for the
        user operation to run.

        @sa notify()
    */
    template <typename Operation>
    std::cv_status wait_for(Operation op, Milliseconds timeout)
    {
        const auto stop_waiting = [this]() -> bool { return m_notified; };

        m_notified = false;
        op();

        // wait:
        std::unique_lock<std::mutex> lock(m_mutex);
        const bool result = m_access_manager.wait_for(lock, timeout, stop_waiting);
        // according to std::condition_variable documentation:
        // * `false` if the predicate stop_waiting still evaluates to false
        //   after the rel_time timeout expired, otherwise `true`
        return result ? std::cv_status::no_timeout : std::cv_status::timeout;
    }

    /*! @brief Notifies a waiting thread that the execution should proceed.

        @sa wait_for()
    */
    void notify() noexcept
    {
        m_notified = true;
        m_access_manager.notify_one();
    }

private:
    std::mutex m_mutex{};
    std::condition_variable m_access_manager{};
    std::atomic_bool m_notified{false};
};

}  // namespace step::utils
