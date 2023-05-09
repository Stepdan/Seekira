#include "camera_distortion.hpp"

#include <core/base/utils/type_utils.hpp>

namespace step::video {

bool CameraDistortion::operator==(const CameraDistortion& rhs) const noexcept
{
    return utils::compare(coefs, rhs.coefs);
}

void CameraDistortion::serialize(ArrayPtrJSON& container)
{
    for (const auto& dist_coef : coefs)
        json::add(container, dist_coef);
}

void CameraDistortion::deserialize(const ArrayPtrJSON& container)
{
    coefs.reserve(container->size());
    json::for_each_in_array<double>(container, [this](double dist_coef) { coefs.push_back(dist_coef); });
}

}  // namespace step::video
