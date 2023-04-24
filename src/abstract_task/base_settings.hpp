#pragma once

#include <base/interfaces/serializable.hpp>

#include <string>

namespace step::task {

constexpr std::string_view CFG_FLD_TASK_SETTINGS_ID = "id";

class BaseSettings : public ISerializable
{
public:
    virtual ~BaseSettings() = default;
    virtual const std::string& get_settings_id() const noexcept { return "AbstractTaskBaseSettings"; }

protected:
    BaseSettings() = default;
};

#define TASK_SETTINGS(SettingsID)                                                                                      \
    static const std::string SETTINGS_ID;                                                                              \
    const std::string& get_settings_id() const noexcept override                                                       \
    {                                                                                                                  \
        return SETTINGS_ID;                                                                                            \
    }

}  // namespace step::task