#pragma once

#include <core/base/interfaces/serializable.hpp>

#include <vector>

namespace step::video {

struct CameraDistortion : public ISerializable
{
    CameraDistortion() = default;
    CameraDistortion(const std::vector<double>& c) : coefs(c) {}

    bool operator==(const CameraDistortion& rhs) const noexcept;
    bool operator!=(const CameraDistortion& rhs) const noexcept { return !(*this == rhs); }

    bool is_empty() const noexcept { return coefs.empty(); }

    void serialize(ArrayPtrJSON&) override;
    void deserialize(const ArrayPtrJSON&) override;

    std::vector<double> coefs;
};

}  // namespace step::video
