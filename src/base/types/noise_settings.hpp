#pragma once

#include <base/interfaces/serializable.hpp>

namespace step {

struct NoiseSettings : public ISerializable
{
    NoiseSettings(double t = 0.0, double a = 0.0) : translation(t), angle(a) {}

    bool operator==(const NoiseSettings& rhs) const noexcept;
    bool operator!=(const NoiseSettings& rhs) const noexcept { return !(*this == rhs); }

    void serialize(ObjectPtrJSON&) override;
    void deserialize(const ObjectPtrJSON&) override;

    double translation;  // meters
    double angle;        // degrees
};

}  // namespace step
