#pragma once

#include <core/task/base_task.hpp>

#include <proc/interfaces/device_type.hpp>

#include <filesystem>

namespace step::proc {

class SettingsNeuralOnnxRuntime : public task::BaseSettings
{
public:
    TASK_SETTINGS(SettingsNeuralOnnxRuntime)

    SettingsNeuralOnnxRuntime() = default;

    bool operator==(const SettingsNeuralOnnxRuntime& rhs) const noexcept;
    bool operator!=(const SettingsNeuralOnnxRuntime& rhs) const noexcept { return !(*this == rhs); }

    void set_model_path(const std::filesystem::path& value) { m_model_path = value; }
    const std::filesystem::path& get_model_path() const noexcept { return m_model_path; }

    void set_device_type(DeviceType type) { m_device_type = type; }
    DeviceType get_device_type() const noexcept { return m_device_type; }

    const std::vector<float>& get_means() const noexcept { return m_means; }
    void set_means(const std::vector<float>& values) { m_means = values; }

    const std::vector<float>& get_norms() const noexcept { return m_norms; }
    void set_norms(const std::vector<float>& values) { m_norms = values; }

public:
    std::filesystem::path m_model_path;
    DeviceType m_device_type{DeviceType::Undefined};

    std::vector<float> m_means;
    std::vector<float> m_norms;
};

std::shared_ptr<task::BaseSettings> create_neural_onnxruntime_settings(const ObjectPtrJSON&);

}  // namespace step::proc