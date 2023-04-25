#pragma once

#include "camera.hpp"

namespace step::video {

inline std::unique_ptr<ICameraSingleshot> create_camera_singleshot(const CameraSettings&)
{
    static_assert("Can't create ICameraSingleshot from CameraSettings");
    return nullptr;
}

inline std::unique_ptr<ICameraStreaming> create_camera_streaming(const CameraSettings&)
{
    static_assert("Can't create ICameraStreaming from CameraSettings");
    return nullptr;
}

}  // namespace step::video
