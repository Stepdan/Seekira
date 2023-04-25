#include "camera_settings.hpp"

#include <base/types/config_fields.hpp>
#include <base/utils/type_utils.hpp>
#include <base/utils/string_utils.hpp>

namespace step::video {

bool CameraSettings::operator==(const CameraSettings& rhs) const noexcept
{
    /* clang-format off */
    return true
        && id                == rhs.id
        && serial_id         == rhs.serial_id
        && pix_fmt           == rhs.pix_fmt
        && distortion        == rhs.distortion
        && extrinsics        == rhs.extrinsics
        && intrinsics        == rhs.intrinsics
        && resolution        == rhs.resolution
        && exposure          == rhs.exposure
        && frame_rate        == rhs.frame_rate
        && reconnect_timeout == rhs.reconnect_timeout
    ;
    /* clang-format off */
}

void CameraSettings::serialize(ObjectPtrJSON& container)
{
    json::set(container, CFG_FLD::ID, id);
    json::set(container, CFG_FLD::SERIAL_ID, serial_id);
    json::set(container, CFG_FLD::PIXEL_FORMAT, step::utils::to_string(pix_fmt));

    // Resolution
    auto resolution_json = json::make_object_json_ptr();
    resolution.serialize(resolution_json);
    json::set(container, CFG_FLD::RESOLUTION, resolution_json);

    // Intrinsics
    auto intrinsics_json = json::make_object_json_ptr();
    intrinsics.serialize(intrinsics_json);
    json::set(container, CFG_FLD::INTRINSICS, intrinsics_json);

    // Distortion
    if (distortion.has_value())
    {
        auto distortion_json = json::make_array_json_ptr();
        distortion.value().serialize(distortion_json);
        json::set(container, CFG_FLD::DISTORTION, distortion_json);
    }

    // Extrinsics
    if (extrinsics.has_value())
    {
        auto extrinsics_json = json::make_object_json_ptr();
        extrinsics.value().serialize(extrinsics_json);
        json::set(container, CFG_FLD::POSE, extrinsics_json);
    }

    json::set_opt(container, CFG_FLD::EXPOSURE, exposure);
    json::set_opt(container, CFG_FLD::FRAME_RATE, frame_rate);

    if (reconnect_timeout.has_value())
        json::set(container, CFG_FLD::RECONNECT_TIMEOUT, reconnect_timeout.value().count());
}

void CameraSettings::deserialize(const ObjectPtrJSON& container)
{
    id = json::get<std::string>(container, CFG_FLD::ID);
    serial_id = json::get<std::string>(container, CFG_FLD::SERIAL_ID);
    step::utils::from_string(pix_fmt,
                       json::get(container, CFG_FLD::PIXEL_FORMAT, step::utils::to_string(video::PixFmt::Undefined)));

    // Resolution
    auto resolution_json = json::get_object(container, CFG_FLD::RESOLUTION);
    resolution.deserialize(resolution_json);

    // Intrinsics
    auto intrinsics_json = json::get_object(container, CFG_FLD::INTRINSICS);
    intrinsics.deserialize(intrinsics_json);

    // Distortion
    auto distortion_json = json::opt_array(container, CFG_FLD::DISTORTION);
    if (distortion_json)
    {
        video::CameraDistortion dist;
        dist.deserialize(distortion_json);
        distortion = dist;
    }

    // Extrinsics
    auto extrinsics_json = json::opt_object(container, CFG_FLD::POSE);
    if (extrinsics_json)
    {
        video::CameraExtrinsics extr;
        extr.deserialize(extrinsics_json);
        extrinsics = extr;
    }

    exposure = json::get_opt<int>(container, CFG_FLD::EXPOSURE);
    frame_rate = json::get_opt<int>(container, CFG_FLD::FRAME_RATE);

    auto rec_timeout = json::get_opt<TimeType>(container, CFG_FLD::RECONNECT_TIMEOUT);
    if (rec_timeout.has_value())
        reconnect_timeout = decltype(reconnect_timeout)(rec_timeout.value());
}

}  // namespace rvision
