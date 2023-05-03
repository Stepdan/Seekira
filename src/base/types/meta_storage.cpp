#include "meta_storage.hpp"

#include <log/log.hpp>

namespace step {

void MetaStorage::set_attachment(const std::string& id, std::any&& attachment)
{
    if (m_storage.contains(id))
        STEP_LOG(L_TRACE, "MetaStorage already has {}, rewrite", id);

    m_storage[id] = std::move(attachment);
}

std::optional<std::any> MetaStorage::get_attachment(const std::string& id)
{
    if (!m_storage.contains(id))
    {
        STEP_LOG(L_TRACE, "MetaStorage doesn't has {}", id);
        return std::nullopt;
    }

    return m_storage[id];
}

}  // namespace step