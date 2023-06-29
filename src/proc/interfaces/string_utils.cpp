#include "face_engine.hpp"

#include <core/exception/assert.hpp>

#include <core/base/types/config_fields.hpp>
#include <core/base/utils/find_pair.hpp>
#include <core/base/utils/string_utils.hpp>
#include <core/base/utils/type_utils.hpp>

#include <utility>

namespace {

/* clang-format off */
constexpr std::pair<step::proc::FaceEngineType, std::string_view> g_face_engine_types[] = {
    { step::proc::FaceEngineType::TDV       , "TDV"         },
    { step::proc::FaceEngineType::OpenVINO  , "OpenVINO"    },
};

constexpr std::pair<step::proc::IFaceEngine::Mode, std::string_view> g_face_engine_modes[] = {
    { step::proc::IFaceEngine::Mode::FE_DETECTION               , "detection"                   },
    { step::proc::IFaceEngine::Mode::FE_RECOGNITION             , "recognition"                 },
    { step::proc::IFaceEngine::Mode::FE_DETECTION_RECOGNITION   , "detection_and_recognition"   },
    { step::proc::IFaceEngine::Mode::FE_ALL                     , "all"                         },
};

constexpr std::pair<step::proc::FaceMatchStatus, std::string_view> g_face_match_statuses[] = {
    { step::proc::FaceMatchStatus::NotMatched, "not matched" },
    { step::proc::FaceMatchStatus::Possible  , "possible"    },
    { step::proc::FaceMatchStatus::Matched   , "matched"     },
};

// constexpr std::pair<step::proc::TrackingEngineType, std::string_view> g_tracking_engine_types[] = {
//     { step::proc::TrackingEngineType::OpenCV, "OpenCV" },
// };
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
std::string to_string(step::proc::IFaceEngine::Mode mode)
{
    if (auto it = step::utils::find_pair_iterator_by_first(g_face_engine_modes, mode);
        it != std::cend(g_face_engine_modes))
        return std::string(it->second);
    else
        return std::string();
}

template <>
std::string to_string(step::proc::FaceMatchStatus status)
{
    return find_by_type(status, g_face_match_statuses);
}

// template <>
// std::string to_string(proc::TrackingEngineType type)
// {
//     return find_by_type(type, g_tracking_engine_types);
// }

template <>
void from_string(proc::FaceEngineType& type, const std::string& str)
{
    find_by_str(str, type, g_face_engine_types);
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

template <>
void from_string(step::proc::FaceMatchStatus& status, const std::string& str)
{
    find_by_str(str, status, g_face_match_statuses);
}

// template <>
// void from_string(proc::TrackingEngineType& type, const std::string& str)
// {
//     find_by_str(str, type, g_tracking_engine_types);
// }

}  // namespace step::utils