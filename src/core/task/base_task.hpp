#pragma once

#include "base_settings.hpp"

#include <core/exception/assert.hpp>

namespace step::task {

class IAbstractTask
{
public:
    virtual ~IAbstractTask() = default;

    virtual const BaseSettings& get_base_settings() const noexcept = 0;
    virtual void set_settings(const BaseSettings& settings) = 0;
    virtual void reset() = 0;
};

class ITaskExtensionDummy
{
public:
    virtual ~ITaskExtensionDummy() = default;
};

template <typename TData, typename TReturnData = void, typename TExt = ITaskExtensionDummy>
class ITask : public IAbstractTask, public TExt
{
public:
    static std::unique_ptr<ITask<TData, TReturnData, TExt>> from_abstract(
        std::unique_ptr<IAbstractTask>&& abstract_task)
    {
        ITask<TData, TReturnData, TExt>* p = dynamic_cast<ITask<TData, TReturnData, TExt>*>(abstract_task.get());
        if (p)
            abstract_task.release();
        return std::unique_ptr<ITask<TData, TReturnData, TExt>>(p);
    }

    static std::shared_ptr<ITask<TData, TReturnData, TExt>> from_abstract(std::shared_ptr<IAbstractTask> abstract_task)
    {
        auto task_ptr = std::dynamic_pointer_cast<ITask<TData, TReturnData, TExt>>(abstract_task);
        STEP_ASSERT(task_ptr, "Invalid shared dynamic_pointer_cast from_abstract");
        return task_ptr;
    }

public:
    virtual ~ITask() = default;

    virtual TReturnData process(TData) = 0;
};

template <typename TSettings, typename TData, typename TReturnData = void, typename TExt = ITaskExtensionDummy>
class BaseTask : public ITask<TData, TReturnData, TExt>
{
protected:
    using SettingsType = TSettings;
    using DataType = TData;
    using ReturnDataType = TReturnData;

protected:
    BaseTask() = default;
    ~BaseTask() = default;

    const BaseSettings& get_base_settings() const noexcept override { return m_typed_settings; }

    virtual void set_settings(const BaseSettings& settings) override
    {
        const SettingsType& typed_settings = dynamic_cast<const SettingsType&>(settings);
        set_settings(typed_settings);
    }

    void reset() override { m_typed_settings = SettingsType(); }

    void set_settings(const SettingsType& settings)
    {
        if (m_typed_settings == settings)
            return;

        reset();
        m_typed_settings = settings;
    }

    virtual ReturnDataType process(DataType) override { STEP_UNDEFINED("BaseTask not implemented!"); }

protected:
    SettingsType m_typed_settings;
};

}  // namespace step::task