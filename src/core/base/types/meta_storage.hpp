#pragma once

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
    std::optional<std::any> get_attachment(const std::string& id) const;

private:
    std::unordered_map<std::string, std::any> m_storage;
};

using MetaStoragePtr = std::shared_ptr<MetaStorage>;

}  // namespace step