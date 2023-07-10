#include "settings_neural_onnxruntime.hpp"

#include <core/base/types/config_fields.hpp>

#include <core/base/utils/string_utils.hpp>

#include <proc/interfaces/face_engine.hpp>

namespace step::proc {

const std::string SettingsNeuralOnnxRuntime::SETTINGS_ID = "SettingsNeuralOnnxRuntime";

bool SettingsNeuralOnnxRuntime::operator==(const SettingsNeuralOnnxRuntime& rhs) const noexcept
{
    /* clang-format off */
    return true
        && m_model_path == rhs.m_model_path
        && m_device_type == rhs.m_device_type
        && m_means == rhs.m_means
        && m_norms == rhs.m_norms
    ;
    /* clang-format on */
}

void SettingsNeuralOnnxRuntime::deserialize(const ObjectPtrJSON& container)
{
    m_model_path = json::get<std::string>(container, CFG_FLD::MODEL_PATH);
    step::utils::from_string(m_device_type, json::get<std::string>(container, CFG_FLD::DEVICE));

    m_means.clear();
    auto means_json = json::opt_array(container, CFG_FLD::MEAN_VALUES);
    if (means_json)
    {
        m_means.reserve(means_json->size());
        json::for_each_in_array<double>(means_json,
                                        [this](double value) { m_means.push_back(static_cast<float>(value)); });
    }

    m_norms.clear();
    auto norm_json = json::opt_array(container, CFG_FLD::NORM_VALUES);
    if (norm_json)
    {
        m_norms.reserve(norm_json->size());
        json::for_each_in_array<double>(norm_json,
                                        [this](double value) { m_norms.push_back(static_cast<float>(value)); });
    }
}

std::shared_ptr<task::BaseSettings> create_neural_onnxruntime_settings(const ObjectPtrJSON& cfg)
{
    auto settings = std::make_shared<SettingsNeuralOnnxRuntime>();
    settings->deserialize(cfg);

    return settings;
}

}  // namespace step::proc