#pragma once

#include "thread_pool_event_handler.hpp"

#include <thread>
#include <functional>

namespace step::threading {

/**
 * @brief Класс политики, реализующий асинхронное исполнение функтора с использованием пула потоков.
 * @tparam threadCount Количество потоков в используемом пуле. По умолчанию имеет значение 1.
 * в этом параметре указан 0, то используется столько потоков, сколько имеет исполнительных ядер в системе.
 */
template <unsigned int threadCount = 1>
class ThreadPoolExecutePolicy
{
public:
    /**
	 * @brief Конструктор.
	 */
    ThreadPoolExecutePolicy() : m_thread_pool(threadCount == 0 ? std::thread::hardware_concurrency() : threadCount) {}

    ThreadPoolExecutePolicy(const ThreadPoolExecutePolicy&) = delete;
    ThreadPoolExecutePolicy& operator=(const ThreadPoolExecutePolicy&) = delete;

    /**
	 * @brief Перегруженный оператор вызова функции.
	 * Добавляет указанный функтор в очередь на исполнени в пуле потоков.
	 */
    void operator()(std::function<void()> functor) { m_thread_pool.add_task(functor); }

private:
    /**
	 * Пул потоков, в котором выполняются задачи, передаваемые объекту политики.
	 */
    ThreadPoolEventHandler m_thread_pool;
};

}  // namespace step::threading
