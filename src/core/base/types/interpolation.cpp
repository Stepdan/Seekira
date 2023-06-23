#include "interpolation.hpp"

#include <core/base/utils/find_pair.hpp>
#include <core/base/utils/string_utils.hpp>

#include <string_view>
#include <utility>

namespace {
/* clang-format off */

constexpr std::pair<step::InterpolationType, std::string_view> g_interpolation_types[] = {
    { step::InterpolationType::Linear   , "linear"  },
    { step::InterpolationType::Cubic    , "cubic"   },
    { step::InterpolationType::Area     , "area"    },
};

/* clang-format on */
}  // namespace

namespace step::utils {

template <>
std::string to_string(step::InterpolationType type)
{
    return find_by_type(type, g_interpolation_types);
}

template <>
void from_string(step::InterpolationType& type, const std::string& str)
{
    find_by_str(str, type, g_interpolation_types);
}

}  // namespace step::utils