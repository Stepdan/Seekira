#include "fake_camera_settings.hpp"

#include <base/types/config_fields.hpp>

namespace step::video {

bool FakeCameraSettings::operator==(const FakeCameraSettings& rhs) const noexcept
{
    /* clang-format off */
    return true
        && CameraSettings::operator==(rhs)
        && source_path == rhs.source_path
        && is_cyclic == rhs.is_cyclic
    ;
    /* clang-format on */
}

void FakeCameraSettings::serialize(ObjectPtrJSON& container)
{
    CameraSettings::serialize(container);
    json::set(container, CFG_FLD::PATH, source_path.string());
    json::set(container, CFG_FLD::CYCLIC, is_cyclic);
}

void FakeCameraSettings::deserialize(const ObjectPtrJSON& container)
{
    CameraSettings::deserialize(container);
    source_path = json::get<std::string>(container, CFG_FLD::PATH);
    is_cyclic = json::get<bool>(container, CFG_FLD::CYCLIC);
}

}  // namespace step::video