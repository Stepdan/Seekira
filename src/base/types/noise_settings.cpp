#include "noise_settings.hpp"

#include <core/utils/json_utils.hpp>
#include <core/utils/type_utils.hpp>

namespace rvision {

bool NoiseSettings::operator==(const NoiseSettings& rhs) const noexcept
{
    return utils::compare(translation, rhs.translation) && utils::compare(angle, rhs.angle);
}

void serialize(const NoiseSettings& data, ObjectPtrJSON& container)
{
    json::set(container, "angle", data.angle);
    json::set(container, "translation", data.translation);
}

void deserialize(const ObjectPtrJSON& container, NoiseSettings& noise)
{
    noise.angle = json::get<double>(container, "angle");
    noise.translation = json::get<double>(container, "translation");
}

}  // namespace rvision
