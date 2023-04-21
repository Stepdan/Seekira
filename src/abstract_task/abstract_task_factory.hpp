#pragma once

#include "base_task.hpp"

#include <base/utils/exception/assert.hpp>

#include <fmt/format.h>

#include <functional>
#include <map>
#include <memory>

namespace step::task {

template <typename TData>
class AbstractTaskFactory
{
public:
    using CreatorUnique = std::function<std::unique_ptr<ITask<TData>>(const BaseSettings&)>;
    using CreatorShared = std::function<std::shared_ptr<ITask<TData>>(const BaseSettings&)>;

public:
    static auto& instance()
    {
        static AbstractTaskFactory<TData> obj;
        return obj;
    }

    void register_creator_unique(const std::string& task_id, CreatorUnique creator)
    {
        STEP_ASSERT(m_creators_unique.contains(task_id), "Abstract task creator unique with id {} already registered!",
                    task_id);

        m_creators_unique[task_id] = creator;
    }

    void register_creator_shared(const std::string& task_id, CreatorShared creator)
    {
        STEP_ASSERT(m_creators_shared.contains(task_id), "Abstract task creator shared with id {} already registered!",
                    task_id);

        m_creators_shared[task_id] = creator;
    }

    std::unique_ptr<ITask<TData>> create_unique(const std::string& task_id, const BaseSettings& settings)
    {
        STEP_ASSERT(!m_creators_unique.contains(task_id), "Abstract task creator unique with id {} was not registered!",
                    task_id);

        return m_creators_unique[task_id](settings);
    }

    std::shared_ptr<ITask<TData>> create_shared(const std::string& task_id, const BaseSettings& settings)
    {
        STEP_ASSERT(!m_creators_shared.contains(task_id), "Abstract task creator shared with id {} was not registered!",
                    task_id);

        return m_creators_shared[task_id](settings);
    }

private:
    AbstractTaskFactory() = default;
    ~AbstractTaskFactory() = default;
    AbstractTaskFactory(const AbstractTaskFactory&) = delete;
    AbstractTaskFactory(AbstractTaskFactory&&) = delete;
    AbstractTaskFactory& operator=(const AbstractTaskFactory&) = delete;
    AbstractTaskFactory& operator=(AbstractTaskFactory&&) = delete;

private:
    std::map<std::string, CreatorUnique, std::less<std::string>> m_creators_unique;
    std::map<std::string, CreatorShared, std::less<std::string>> m_creators_shared;
};

}  // namespace step::task

#define REGISTER_ABSTRACT_TASK_CREATOR_UNIQUE(DATA_TYPE, TASK_ID, CREATOR)                                             \
    AbstractTaskFactory<DATA_TYPE>::instance().register_creator_unique(TASK_ID, CREATOR);

#define REGISTER_ABSTRACT_TASK_CREATOR_SHARED(DATA_TYPE, TASK_ID, CREATOR)                                             \
    AbstractTaskFactory<DATA_TYPE>::instance().register_creator_shared(TASK_ID, CREATOR);