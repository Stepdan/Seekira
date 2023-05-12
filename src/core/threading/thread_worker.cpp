#include "thread_worker.hpp"

#include <core/log/log.hpp>

namespace step::threading {

ThreadWorker::ThreadWorker() = default;

ThreadWorker::~ThreadWorker()
{
    STEP_LOG(L_TRACE, "ThreadWorker destruction");
    stop_worker();
}

void ThreadWorker::run_worker()
{
    if (m_is_running)
        return;

    m_worker = std::thread(&ThreadWorker::worker_thread, this);
    m_is_running.store(true);
    STEP_LOG(L_TRACE, "ThreadWorker has been started");
}

void ThreadWorker::stop_worker()
{
    if (!m_is_running)
        return;

    STEP_LOG(L_TRACE, "Stopping ThreadWorker");
    m_need_stop.store(true);

    if (m_worker.joinable())
        m_worker.join();

    reset_exception();

    m_is_running.store(false);
    m_need_stop.store(false);

    STEP_LOG(L_TRACE, "ThreadWorker has been stopped");
}

}  // namespace step::threading