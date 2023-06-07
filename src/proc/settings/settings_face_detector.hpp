#pragma once

#include <core/task/base_task.hpp>

namespace step::proc {

class SettingsFaceDetector : public task::BaseSettings
{
public:
    enum class Algorithm
    {
        TDV,  // 3DIVI
    };

public:
    TASK_SETTINGS(SettingsFaceDetector)

    SettingsFaceDetector() = default;

    bool operator==(const SettingsFaceDetector& rhs) const noexcept;
    bool operator!=(const SettingsFaceDetector& rhs) const noexcept { return !(*this == rhs); }

    Algorithm get_algorithm() const noexcept { return m_alg_type; }
    const std::string& get_model_path() const noexcept { return m_model_path; }

    void set_algorithm(Algorithm value) { m_alg_type = value; }
    void set_model_path(const std::string& model_path) { m_model_path = model_path; }

public:
    Algorithm m_alg_type;
    std::string m_model_path;
};

}  // namespace step::proc