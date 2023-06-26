#include "face_engine.hpp"

#include <core/base/types/config_fields.hpp>
#include <core/base/utils/type_utils.hpp>

namespace step::proc {

FaceMatchResult::FaceMatchResult(double prob_value, double prob_threshold /*= 1.0*/) : probability(prob_value)
{
    status = step::utils::compare(probability, 1.0) ? FaceMatchStatus::Matched
             : probability > prob_threshold         ? FaceMatchStatus::Possible
                                                    : FaceMatchStatus::NotMatched;
}

void IFaceEngine::Initializer::deserialize(const ObjectPtrJSON& container)
{
    models_path = json::get<std::string>(container, CFG_FLD::MODEL_PATH);
    step::utils::from_string<FaceEngineType>(type, json::get<std::string>(container, CFG_FLD::TYPE));
    step::utils::from_string<IFaceEngine::Mode>(mode, json::get<std::string>(container, CFG_FLD::MODE));
    save_frames = json::get<bool>(container, CFG_FLD::FACE_ENGINE_INIT_SAVE_FRAMES);
    step::utils::from_string<DeviceType>(device, json::get<std::string>(container, CFG_FLD::DEVICE));
    match_gt_threshold = json::get<double>(container, CFG_FLD::FACE_MATCHING_GROUNDTRUTH_THRESHOLD);
    match_gf_threshold = json::get<double>(container, CFG_FLD::FACE_MATCHING_GROUNDFALSE_THRESHOLD);
    match_prob_threshold = json::get<double>(container, CFG_FLD::FACE_MATCHING_PROBABILITY_THRESHOLD);

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
        && !step::utils::compare(match_gt_threshold, 0.0)
        && !step::utils::compare(match_gf_threshold, 0.0)
        && !step::utils::compare(match_prob_threshold, 0.0)
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
        && step::utils::compare(match_gt_threshold, rhs.match_gt_threshold)
        && step::utils::compare(match_gf_threshold, rhs.match_gf_threshold)
        && step::utils::compare(match_prob_threshold, rhs.match_prob_threshold)
    ;
    /* clang-format on */
}

double BaseFaceEngine::calc_match_probability(double distance) const noexcept
{
    // Базовый подсчет вероятности совпадения лица с искомым.
    // Наследники могут переопределить механизм подсчета.
    if (distance < m_match_gt_threshold)
        return 1.0;

    if (distance > m_match_gf_threshold)
        return 0.0;

    return (m_match_gf_threshold - distance) / (m_match_gf_threshold - m_match_gt_threshold);
}

}  // namespace step::proc