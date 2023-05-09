#pragma once

#include <core/base/interfaces/serializable.hpp>

namespace step::video {

struct CameraIntrinsics : public ISerializable
{
    CameraIntrinsics() = default;
    CameraIntrinsics(double _fx, double _fy, double _cx, double _cy) : fx(_fx), fy(_fy), cx(_cx), cy(_cy) {}

    bool operator==(const CameraIntrinsics& rhs) const noexcept;
    bool operator!=(const CameraIntrinsics& rhs) const noexcept { return !(*this == rhs); }

    void serialize(ObjectPtrJSON&) override;
    void deserialize(const ObjectPtrJSON&) override;

    double fx{0.0};
    double fy{0.0};
    double cx{0.0};
    double cy{0.0};
};

}  // namespace step::video
