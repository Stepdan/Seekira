#include "meta_storage.hpp"

#include <core/log/log.hpp>

namespace step {

void MetaStorage::set_attachment(const std::string& id, std::any&& attachment)
{
    if (m_storage.contains(id))
        STEP_LOG(L_TRACE, "MetaStorage already has {}, rewrite", id);

    m_storage[id] = std::move(attachment);
}

std::any MetaStorage::get_attachment_impl(const std::string& id) const
{
    if (auto it = m_storage.find(id); it != m_storage.cend())
        return it->second;

    STEP_LOG(L_TRACE, "MetaStorage doesn't has {}", id);
    return std::any();
}

}  // namespace step