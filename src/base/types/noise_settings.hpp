#pragma once

#include <core/utils/json_types.hpp>

namespace rvision {

struct NoiseSettings
{
    NoiseSettings(double t = 0.0, double a = 0.0) : translation(t), angle(a) {}

    bool operator==(const NoiseSettings& rhs) const noexcept;
    bool operator!=(const NoiseSettings& rhs) const noexcept { return !(*this == rhs); }

    double translation;  // meters
    double angle;        // degrees
};

void serialize(const NoiseSettings&, ObjectPtrJSON&);

void deserialize(const ObjectPtrJSON&, NoiseSettings&);

}  // namespace rvision
