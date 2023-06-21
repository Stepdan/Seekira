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

    // Если есть conn_id, то face_engine будет получен через механизм IFaceEngineUser
    void set_face_engine_conn_id(const std::string& value) { m_face_engine_conn_id = value; }
    const std::string& get_face_engine_conn_id() const noexcept { return m_face_engine_conn_id; }

    void set_face_engine_init(const IFaceEngine::Initializer& init) { m_face_engine_init = init; }
    IFaceEngine::Initializer get_face_engine_init() const noexcept { return m_face_engine_init; }

public:
    IFaceEngine::Initializer m_face_engine_init;
    std::string m_face_engine_conn_id;
};

std::shared_ptr<task::BaseSettings> create_face_detector_settings(const ObjectPtrJSON&);

}  // namespace step::proc