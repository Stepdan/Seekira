#pragma once

#include "base_settings.hpp"

#include <core/base/types/config_fields.hpp>
#include <core/base/json/json_utils.hpp>

#include <functional>
#include <map>
#include <memory>

namespace step::task {

class TaskSettingsFactory
{
public:
    using Creator = std::function<std::shared_ptr<BaseSettings>(const ObjectPtrJSON&)>;

public:
    static auto& instance()
    {
        static TaskSettingsFactory obj;
        return obj;
    }

    void register_creator(const std::string& id, Creator creator)
    {
        STEP_ASSERT(!m_creators.contains(id), "Task settings creator with id {} already registered!", id);

        m_creators[id] = creator;
    }

    std::shared_ptr<BaseSettings> create(const ObjectPtrJSON& json_obj)
    {
        const auto id = json::get<std::string>(json_obj, CFG_FLD::TASK_SETTINGS_ID);
        STEP_ASSERT(m_creators.contains(id), "Task settings creator with id {} was not registered!", id);

        return m_creators[id](json_obj);
    }

private:
    TaskSettingsFactory() = default;
    ~TaskSettingsFactory() = default;
    TaskSettingsFactory(const TaskSettingsFactory&) = delete;
    TaskSettingsFactory(TaskSettingsFactory&&) = delete;
    TaskSettingsFactory& operator=(const TaskSettingsFactory&) = delete;
    TaskSettingsFactory& operator=(TaskSettingsFactory&&) = delete;

private:
    std::map<std::string, Creator, std::less<std::string>> m_creators;
};

}  // namespace step::task

#define REGISTER_TASK_SETTINGS_CREATOR(ID, CREATOR)                                                                    \
    step::task::TaskSettingsFactory::instance().register_creator(ID, CREATOR);