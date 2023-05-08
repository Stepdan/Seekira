#include "thread_pool_event_handler.hpp"

// Объявление версии Windows нужно для boost::asio. Если убрать следующий блок #ifndef...#endif, при сборке компилятор выведет предупреждение.
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#endif

#include <boost/asio/io_service.hpp>
#include <boost/thread/thread.hpp>

// Включение windows.h нужно для boost::asio.
#ifdef _WIN32
#include "windows.h"
#endif

#include <base/utils/exception/assert.hpp>

#include <log/log.hpp>

namespace step {

std::unique_ptr<ThreadPoolEventHandler> g_thread_pool;

struct ThreadPoolEventHandler::Impl
{
    Impl(unsigned int threadCount) : m_work(m_service)
    {
        for (unsigned int index = 0; index < threadCount; ++index)
            m_threads.create_thread(boost::bind(&boost::asio::io_service::run, &m_service));
    }
    /// Планировщик.
    boost::asio::io_service m_service;
    /// Держит планировщик в рабочем состоянии.
    boost::asio::io_service::work m_work;
    /// Рабочие потоки.
    boost::thread_group m_threads;
};

void set_global_thread_pool(std::unique_ptr<ThreadPoolEventHandler>&& thread_pool)
{
    g_thread_pool = std::move(thread_pool);
}

ThreadPoolEventHandler& get_global_thread_pool()
{
    STEP_ASSERT(g_thread_pool, "Global thread pool wasn't set");

    return *g_thread_pool;
}

//..............................................................................

ThreadPoolEventHandler::ThreadPoolEventHandler(unsigned int threadCount) : m_impl(std::make_unique<Impl>(threadCount))
{
}

//..............................................................................

ThreadPoolEventHandler::~ThreadPoolEventHandler(void) { stop(); }

//..............................................................................

void ThreadPoolEventHandler::stop()
{
    m_impl->m_service.stop();
    m_impl->m_threads.join_all();
}

void ThreadPoolEventHandler::add_task(const std::function<void()>& task) { m_impl->m_service.post(std::bind(task)); }

//..............................................................................

}  // namespace step