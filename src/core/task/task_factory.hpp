#pragma once

#include "base_task.hpp"

#include <core/exception/assert.hpp>

#include <fmt/format.h>

#include <functional>
#include <map>
#include <memory>

namespace step::task {

template <typename TData>
class TaskFactory
{
public:
    using CreatorUnique = std::function<std::unique_ptr<ITask<TData>>(const std::shared_ptr<task::BaseSettings>&)>;
    using CreatorShared = std::function<std::shared_ptr<ITask<TData>>(const std::shared_ptr<task::BaseSettings>&)>;

public:
    static auto& instance()
    {
        static TaskFactory<TData> obj;
        return obj;
    }

    void register_creator_unique(const std::string& task_settings_id, CreatorUnique creator)
    {
        STEP_ASSERT(!m_creators_unique.contains(task_settings_id), "Task creator unique with id {} already registered!",
                    task_settings_id);

        m_creators_unique[task_settings_id] = creator;
    }

    void register_creator_shared(const std::string& task_settings_id, CreatorShared creator)
    {
        STEP_ASSERT(!m_creators_shared.contains(task_settings_id), "Task creator shared with id {} already registered!",
                    task_settings_id);

        m_creators_shared[task_settings_id] = creator;
    }

    std::unique_ptr<ITask<TData>> create_unique(const std::shared_ptr<BaseSettings>& settings)
    {
        STEP_ASSERT(settings, "Task creator unique can't create from empty settings!");
        STEP_ASSERT(m_creators_unique.contains(settings->get_settings_id()),
                    "Task creator unique with id {} was not registered!", settings->get_settings_id());

        return m_creators_unique[settings->get_settings_id()](settings);
    }

    std::shared_ptr<ITask<TData>> create_shared(const std::shared_ptr<BaseSettings>& settings)
    {
        STEP_ASSERT(settings, "Task creator shared can't create from empty settings!");
        STEP_ASSERT(m_creators_shared.contains(settings->get_settings_id()),
                    "Task creator shared with id {} was not registered!", settings->get_settings_id());

        return m_creators_shared[settings->get_settings_id()](settings);
    }

private:
    TaskFactory() = default;
    ~TaskFactory() = default;
    TaskFactory(const TaskFactory&) = delete;
    TaskFactory(TaskFactory&&) = delete;
    TaskFactory& operator=(const TaskFactory&) = delete;
    TaskFactory& operator=(TaskFactory&&) = delete;

private:
    std::map<std::string, CreatorUnique, std::less<std::string>> m_creators_unique;
    std::map<std::string, CreatorShared, std::less<std::string>> m_creators_shared;
};

}  // namespace step::task

#define REGISTER_TASK_CREATOR_UNIQUE(DATA_TYPE, TASK_SETTINGS_ID, CREATOR)                                             \
    step::task::TaskFactory<DATA_TYPE>::instance().register_creator_unique(TASK_SETTINGS_ID, CREATOR);

#define REGISTER_TASK_CREATOR_SHARED(DATA_TYPE, TASK_SETTINGS_ID, CREATOR)                                             \
    step::task::TaskFactory<DATA_TYPE>::instance().register_creator_shared(TASK_SETTINGS_ID, CREATOR);