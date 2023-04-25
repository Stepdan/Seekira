#include "camera_intrinsics.hpp"

#include <base/types/config_fields.hpp>
#include <base/utils/type_utils.hpp>

namespace step::video {

bool CameraIntrinsics::operator==(const CameraIntrinsics& rhs) const noexcept
{
    /* clang-format off */
    return true
        && utils::compare(fx, rhs.fx)
        && utils::compare(fy, rhs.fy)
        && utils::compare(cx, rhs.cx)
        && utils::compare(cy, rhs.cy)
    ;
    /* clang-format off */
}

void CameraIntrinsics::serialize(ObjectPtrJSON& container)
{
    json::set(container, CFG_FLD::FX, fx);
    json::set(container, CFG_FLD::FY, fy);
    json::set(container, CFG_FLD::CX, cx);
    json::set(container, CFG_FLD::CY, cy);
}

void CameraIntrinsics::deserialize(const ObjectPtrJSON& container)
{
    fx = json::get<double>(container, CFG_FLD::FX);
    fy = json::get<double>(container, CFG_FLD::FY);
    cx = json::get<double>(container, CFG_FLD::CX);
    cy = json::get<double>(container, CFG_FLD::CY);
}

}  // namespace rvision
