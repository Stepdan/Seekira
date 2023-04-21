#pragma once

#include "base_settings.hpp"

#include <base/utils/exception/throw_utils.hpp>

namespace step::task {

template <typename TData>
class ITask
{
public:
    virtual ~ITask() = default;

    virtual const BaseSettings& get_base_settings() const noexcept = 0;
    virtual void set_settings(const BaseSettings& settings) = 0;
    virtual void reset() = 0;

    virtual void process(TData) = 0;
};

template <typename TData, typename TSettings>
class BaseTask : public ITask<TData>
{
protected:
    using DataType = TData;
    using SettingsType = TSettings;

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

    virtual void process(DataType) override { utils::throw_runtime_with_log("BaseTask not implemented!"); }

protected:
    SettingsType m_typed_settings;
};

}  // namespace step::task