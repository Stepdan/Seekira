#pragma once

#include <functional>
#include <memory>

namespace step {

/**
@brief Пул потоков

Простой пул потоков, принимающий задачи в формате функторов.

Этот класс использует библиотеку boost::asio и является, по сути, обёрткой вокруг планировщика задач boost::asio::io_service. Конструктор класса порождает заданное количество рабочих потоков, пул готов к работе сразу после конструирования.

Функтор-класс задачи должен удовлетворять следующим требованиям:
- в нём должен быть объявлен оператор operator()(), выполняющий работу;
- в нём должен быть typedef void result_type;
- экземпляры класса должны быть копируемыми.

@sa example_thread_pool
*/
class ThreadPoolEventHandler
{
public:
    ThreadPoolEventHandler(unsigned int threadCount);
    virtual ~ThreadPoolEventHandler();
    ThreadPoolEventHandler(const ThreadPoolEventHandler&) = delete;
    ThreadPoolEventHandler& operator=(const ThreadPoolEventHandler&) = delete;

    /**
	@brief Добавляет задачу.
	@tparam Task класс задачи.

	Метод создаёт копию объекта task, которая используется при фактическом запуске задачи.
	*/
    void add_task(const std::function<void()>& task);

    /// Останавливает планировщик и дожидается завершения всех задач.
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * @brief Установка глобального объекта ThreadPool.
 */
void set_global_thread_pool(std::unique_ptr<ThreadPoolEventHandler>&& thread_pool);

/**
 * @brief Получение доступа к глобальному объекту ThreadPool.
 */
ThreadPoolEventHandler& get_global_thread_pool();

}  // namespace step
