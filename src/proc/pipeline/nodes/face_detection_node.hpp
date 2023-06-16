#pragma once

#include <core/log/log.hpp>

#include <proc/pipeline/pipeline_task.hpp>

#include <proc/settings/settings_face_detector.hpp>

namespace step::proc {

class FaceDetectionNodeSettings : public task::BaseSettings
{
public:
    TASK_SETTINGS(FaceDetectionNodeSettings)

    FaceDetectionNodeSettings() = default;

    bool operator==(const FaceDetectionNodeSettings& rhs) const noexcept { return false; }
    bool operator!=(const FaceDetectionNodeSettings& rhs) const noexcept { return !(*this == rhs); }

    std::shared_ptr<task::BaseSettings> get_face_detector_settings_base() const noexcept
    {
        return m_face_detector_settings;
    }

private:
    std::shared_ptr<task::BaseSettings> m_face_detector_settings;
};

std::shared_ptr<task::BaseSettings> create_face_detection_node_settings(const ObjectPtrJSON&);

std::unique_ptr<task::IAbstractTask> create_face_detection_node(const std::shared_ptr<task::BaseSettings>& settings);

}  // namespace step::proc