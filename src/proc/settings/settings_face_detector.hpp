#pragma once

#include <core/task/base_task.hpp>

namespace step::proc {

class SettingsFaceDetector : public task::BaseSettings
{
public:
    enum class Algorithm
    {
        _3DIVI
    };

public:
    TASK_SETTINGS(SettingsFaceDetector)

    SettingsFaceDetector() = default;

    bool operator==(const SettingsFaceDetector& rhs) const noexcept;
    bool operator!=(const SettingsFaceDetector& rhs) const noexcept { return !(*this == rhs); }

    Algorithm get_algorithm() const noexcept { return m_alg_type; }

    void set_algorithm(Algorithm value) { m_alg_type = value; }

public:
    Algorithm m_alg_type;
};

}  // namespace step::proc