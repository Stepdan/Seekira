#pragma once

#include <base/types/pose.hpp>

#include <optional>

namespace step::video {

struct CameraExtrinsics
{
    CameraExtrinsics() = default;
    CameraExtrinsics(const Pose& _pose) : pose(_pose) {}

    bool operator==(const CameraExtrinsics& rhs) const noexcept;
    bool operator!=(const CameraExtrinsics& rhs) const noexcept { return !(*this == rhs); }

    void serialize(ObjectPtrJSON&);
    void deserialize(const ObjectPtrJSON&);

    Pose pose;
};

}  // namespace step::video
