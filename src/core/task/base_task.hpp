#pragma once

#include "base_settings.hpp"

#include <core/exception/assert.hpp>

namespace step::task {

template <typename TData, typename TReturnData = void>
class ITask
{
public:
    virtual ~ITask() = default;

    virtual const BaseSettings& get_base_settings() const noexcept = 0;
    virtual void set_settings(const BaseSettings& settings) = 0;
    virtual void reset() = 0;

    virtual TReturnData process(TData) = 0;
};

template <typename TSettings, typename TData, typename TReturnData = void>
class BaseTask : public ITask<TData, TReturnData>
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