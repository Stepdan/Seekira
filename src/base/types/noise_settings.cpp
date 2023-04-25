#include "noise_settings.hpp"

#include <base/types/config_fields.hpp>
#include <base/utils/type_utils.hpp>

namespace step {

bool NoiseSettings::operator==(const NoiseSettings& rhs) const noexcept
{
    return utils::compare(translation, rhs.translation) && utils::compare(angle, rhs.angle);
}

void NoiseSettings::serialize(ObjectPtrJSON& container)
{
    json::set(container, CFG_FLD::ANGLE, angle);
    json::set(container, CFG_FLD::TRANSLATION, translation);
}

void NoiseSettings::deserialize(const ObjectPtrJSON& container)
{
    angle = json::get<double>(container, CFG_FLD::ANGLE);
    translation = json::get<double>(container, CFG_FLD::TRANSLATION);
}

}  // namespace step
