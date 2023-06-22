#include "device_type.hpp"

#include <core/base/utils/find_pair.hpp>
#include <core/base/utils/string_utils.hpp>

#include <string_view>
#include <utility>

namespace {
/* clang-format off */

constexpr std::pair<step::proc::DeviceType, std::string_view> g_device_types[] = {
    { step::proc::DeviceType::CPU, "cpu"    },
    { step::proc::DeviceType::GPU, "gpu"    },
};

/* clang-format on */
}  // namespace

namespace step::utils {

template <>
std::string to_string(step::proc::DeviceType type)
{
    return find_by_type(type, g_device_types);
}

template <>
void from_string(step::proc::DeviceType& type, const std::string& str)
{
    find_by_str(str, type, g_device_types);
}

}  // namespace step::utils