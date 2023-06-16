#pragma once

#include <core/log/log.hpp>

#include <any>
#include <optional>
#include <memory>
#include <string>
#include <unordered_map>

namespace step {

class MetaStorage
{
public:
    void set_attachment(const std::string& id, std::any&& attachment);

    template <typename T>
    std::optional<T> get_attachment(const std::string& id) const noexcept
    {
        auto attachment = get_attachment_impl(id);
        if (!attachment.has_value())
            return std::nullopt;

        try
        {
            return std::any_cast<T>(attachment);
        }
        catch (const std::bad_any_cast& e)
        {
            STEP_LOG(L_ERROR, "Invalid any_cast {} in meta storage!", id);
            return std::nullopt;
        }
    }

private:
    std::any get_attachment_impl(const std::string& id) const;

private:
    std::unordered_map<std::string, std::any> m_storage;
};

using MetaStoragePtr = std::shared_ptr<MetaStorage>;

}  // namespace step