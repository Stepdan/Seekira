#pragma once

#include <core/task/base_task.hpp>

namespace step::proc {

class SettingsPersonDetector : public task::BaseSettings
{
public:
    TASK_SETTINGS(SettingsPersonDetector)

    SettingsPersonDetector() = default;

    bool operator==(const SettingsPersonDetector& rhs) const noexcept;
    bool operator!=(const SettingsPersonDetector& rhs) const noexcept { return !(*this == rhs); }

    void set_model_path(const std::string& value) { m_model_path = value; }
    const std::string& get_model_path() const noexcept { return m_model_path; }

public:
    std::string m_model_path;
};

std::shared_ptr<task::BaseSettings> create_person_detector_settings(const ObjectPtrJSON&);

}  // namespace step::proc