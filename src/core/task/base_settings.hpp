#pragma once

#include <core/base/interfaces/serializable.hpp>

#include <string>

namespace step::task {

class BaseSettings : public ISerializable
{
public:
    virtual ~BaseSettings() = default;
    virtual const std::string& get_settings_id() const noexcept = 0;
    virtual BaseSettings* clone() const = 0;

    std::unique_ptr<BaseSettings> clone_unique() const { return std::unique_ptr<BaseSettings>(this->clone()); }
    std::shared_ptr<BaseSettings> clone_shared() const { return std::shared_ptr<BaseSettings>(this->clone()); }

protected:
    BaseSettings() = default;
};

#define TASK_SETTINGS(ClassName)                                                                                       \
    static const std::string SETTINGS_ID;                                                                              \
    const std::string& get_settings_id() const noexcept override { return SETTINGS_ID; }                               \
    virtual BaseSettings* clone() const override { return (BaseSettings*)new ClassName(*this); }                       \
    ClassName(const ObjectPtrJSON& cfg) { deserialize(cfg); }                                                          \
    void deserialize(const ObjectPtrJSON& cfg) override;                                                               \
    ~ClassName() = default;

}  // namespace step::task