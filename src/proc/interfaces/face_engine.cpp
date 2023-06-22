#include "face_engine.hpp"

#include <core/base/types/config_fields.hpp>
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

namespace step::proc {

void IFaceEngine::Initializer::deserialize(const ObjectPtrJSON& container)
{
    models_path = json::get<std::string>(container, CFG_FLD::MODEL_PATH);
    step::utils::from_string<FaceEngineType>(type, json::get<std::string>(container, CFG_FLD::TYPE));
    step::utils::from_string<IFaceEngine::Mode>(mode, json::get<std::string>(container, CFG_FLD::MODE));
    save_frames = json::get<bool>(container, CFG_FLD::FACE_ENGINE_INIT_SAVE_FRAMES);
    step::utils::from_string<DeviceType>(device, json::get<std::string>(container, CFG_FLD::DEVICE));

    STEP_ASSERT(is_valid(), "FaceEngine is invalid after deserialization!");
}

bool IFaceEngine::Initializer::is_valid() const noexcept
{
    /* clang-format off */
    return true
        && type != FaceEngineType::Undefined
        && mode != Mode::FE_UNDEFINED
        && device != DeviceType::Undefined
        && !models_path.empty()
    ;
    /* clang-format on */
}

bool IFaceEngine::Initializer::operator==(const IFaceEngine::Initializer& rhs) const noexcept
{
    /* clang-format off */
    return true
        && type == rhs.type
        && models_path == rhs.models_path
        && mode == rhs.mode
        && save_frames == rhs.save_frames
        && device == rhs.device
    ;
    /* clang-format on */
}

}  // namespace step::proc