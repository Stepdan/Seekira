#include "camera_resolution.hpp"

#include <core/base/types/config_fields.hpp>

namespace step::video {

bool CameraResolution::operator==(const CameraResolution& rhs) const noexcept
{
    /* clang-format off */
    return true
        && width  == rhs.width
        && height == rhs.height
    ;
    /* clang-format on */
}

void CameraResolution::serialize(ObjectPtrJSON& container)
{
    json::set(container, CFG_FLD::WIDTH, width);
    json::set(container, CFG_FLD::HEIGHT, height);
}

void CameraResolution::deserialize(const ObjectPtrJSON& container)
{
    width = json::get<size_t>(container, CFG_FLD::WIDTH);
    height = json::get<size_t>(container, CFG_FLD::HEIGHT);
}

}  // namespace step::video
