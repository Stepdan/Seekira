#pragma once

#include <core/task/base_task.hpp>

#include <proc/interfaces/face_engine.hpp>

namespace step::proc {

class SettingsFaceDetector : public task::BaseSettings
{
public:
    TASK_SETTINGS(SettingsFaceDetector)

    SettingsFaceDetector() = default;

    bool operator==(const SettingsFaceDetector& rhs) const noexcept;
    bool operator!=(const SettingsFaceDetector& rhs) const noexcept { return !(*this == rhs); }

    FaceEngineType get_face_engine_type() const noexcept { return m_face_engine_type; }
    const std::string& get_model_path() const noexcept { return m_model_path; }
    IFaceEngine::Mode get_mode() const noexcept { return m_mode; }

    void set_face_engine_type(FaceEngineType value) { m_face_engine_type = value; }
    void set_model_path(const std::string& model_path) { m_model_path = model_path; }
    void set_mode(IFaceEngine::Mode mode) { m_mode = mode; }

public:
    FaceEngineType m_face_engine_type;
    IFaceEngine::Mode m_mode;
    std::string m_model_path;
};

std::shared_ptr<task::BaseSettings> create_face_detector_settings(const ObjectPtrJSON&);

}  // namespace step::proc