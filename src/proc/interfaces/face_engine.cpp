#include "face_engine.hpp"

#include <core/base/utils/find_pair.hpp>
#include <core/base/utils/string_utils.hpp>

#include <utility>

namespace {

/* clang-format off */

constexpr std::pair<step::proc::IFaceEngine::Mode, std::string_view> g_face_engine_modes[] = {
    { step::proc::IFaceEngine::Mode::FE_DETECTION               , "detection"                   },
    { step::proc::IFaceEngine::Mode::FE_RECOGNITION             , "recognition"                 },
    { step::proc::IFaceEngine::Mode::FE_DETECTION_RECOGNITION   , "detection_and_recognition"   },
    { step::proc::IFaceEngine::Mode::FE_ALL                     , "all"                         },
};
/* clang-format on */

}  // namespace

// to_string
namespace step::utils {

template <>
std::string to_string(step::proc::IFaceEngine::Mode mode)
{
    if (auto it = step::utils::find_pair_iterator_by_first(g_face_engine_modes, mode);
        it != std::cend(g_face_engine_modes))
        return std::string(it->second);
    else
        return std::string();
}

template <>
void from_string(step::proc::IFaceEngine::Mode& type, const std::string& str)
{
    if (auto it = step::utils::find_pair_iterator_by_second(g_face_engine_modes, str);
        it != std::cend(g_face_engine_modes))
        type = it->first;
    else
        type = step::proc::IFaceEngine::Mode::FE_UNDEFINED;
}

}  // namespace step::utils