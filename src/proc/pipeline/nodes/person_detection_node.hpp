#pragma once

#include <core/log/log.hpp>

#include <proc/pipeline/pipeline_task.hpp>

#include <proc/settings/settings_person_detector.hpp>

namespace step::proc {

class PersonDetectionNodeSettings : public task::BaseSettings
{
public:
    TASK_SETTINGS(PersonDetectionNodeSettings)

    PersonDetectionNodeSettings() = default;

    bool operator==(const PersonDetectionNodeSettings& rhs) const noexcept { return false; }
    bool operator!=(const PersonDetectionNodeSettings& rhs) const noexcept { return !(*this == rhs); }

    std::shared_ptr<task::BaseSettings> get_person_detector_settings_base() const noexcept
    {
        return m_person_detector_settings;
    }

private:
    std::shared_ptr<task::BaseSettings> m_person_detector_settings;
};

std::shared_ptr<task::BaseSettings> create_person_detection_node_settings(const ObjectPtrJSON&);

std::unique_ptr<task::IAbstractTask> create_person_detection_node(const std::shared_ptr<task::BaseSettings>& settings);

}  // namespace step::proc