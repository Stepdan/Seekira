#pragma once

#include "camera_distortion.hpp"
#include "camera_intrinsics.hpp"
#include "camera_extrinsics.hpp"
#include "camera_resolution.hpp"

#include <video/frame/interfaces/pixel_format.hpp>

#include <base/interfaces/serializable.hpp>
#include <base/types/time.hpp>

#include <optional>

namespace step::video {

struct CameraSettings : public ISerializable
{
    std::string id;
    std::string serial_id;
    PixFmt pix_fmt{PixFmt::BGR};

    CameraIntrinsics intrinsics{};
    CameraResolution resolution{};

    std::optional<CameraDistortion> distortion;
    std::optional<CameraExtrinsics> extrinsics;

    // TODO set default values for our purposes
    std::optional<int> exposure;    // TODO: chrono::duration?
    std::optional<int> frame_rate;  // per second

    std::optional<Seconds> reconnect_timeout;

    bool operator==(const CameraSettings& rhs) const noexcept;
    bool operator!=(const CameraSettings& rhs) const noexcept { return !(*this == rhs); }

    void serialize(ObjectPtrJSON& container) override;
    void deserialize(const ObjectPtrJSON& container) override;
};

}  // namespace step::video
