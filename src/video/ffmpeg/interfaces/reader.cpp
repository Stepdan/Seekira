#include "reader.hpp"

#include <core/base/utils/find_pair.hpp>
#include <core/base/utils/string_utils.hpp>

#include <utility>

namespace {

/* clang-format off */
constexpr std::pair<step::video::ff::ReaderMode, std::string_view> g_reader_modes[] = {
    { step::video::ff::ReaderMode::All      , "all"         },
    { step::video::ff::ReaderMode::KeyFrame , "keyframe"    },
};
/* clang-format on */

}  // namespace

// to_string
namespace step::utils {

template <>
std::string to_string(step::video::ff::ReaderMode mode)
{
    return find_by_type(mode, g_reader_modes);
}

template <>
void from_string(step::video::ff::ReaderMode& mode, const std::string& str)
{
    find_by_str(str, mode, g_reader_modes);
}

}  // namespace step::utils