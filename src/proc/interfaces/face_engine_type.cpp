#include "face_engine_type.hpp"

#include <core/exception/assert.hpp>

#include <core/base/utils/find_pair.hpp>
#include <core/base/utils/string_utils.hpp>

#include <utility>

namespace {

/* clang-format off */
constexpr std::pair<step::proc::FaceEngineType, std::string_view> g_face_engine_types[] = {
    { step::proc::FaceEngineType::TDV, "TDV" },
};
/* clang-format on */

}  // namespace

// to_string
namespace step::utils {

template <>
std::string to_string(proc::FaceEngineType type)
{
    return find_by_type(type, g_face_engine_types);
}

template <>
void from_string(proc::FaceEngineType& type, const std::string& str)
{
    find_by_str(str, type, g_face_engine_types);
}

}  // namespace step::utils