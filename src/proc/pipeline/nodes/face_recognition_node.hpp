#pragma once

#include <core/log/log.hpp>

#include <proc/pipeline/pipeline_task.hpp>

namespace step::proc {

class FaceRecognitionNodeSettings : public task::BaseSettings
{
public:
    TASK_SETTINGS(FaceRecognitionNodeSettings)

    FaceRecognitionNodeSettings() = default;

    bool operator==(const FaceRecognitionNodeSettings& rhs) const noexcept { return false; }
    bool operator!=(const FaceRecognitionNodeSettings& rhs) const noexcept { return !(*this == rhs); }

    void set_face_engine_conn_id(const std::string& value) { m_face_engine_conn_id = value; }
    const std::string& get_face_engine_conn_id() const noexcept { return m_face_engine_conn_id; }

    bool get_skip_flag() const noexcept { return m_skip_flag; }
    void set_skip_flag(bool value) { m_skip_flag = true; }

private:
    std::string m_face_engine_conn_id;
    bool m_skip_flag{false};
};

std::shared_ptr<task::BaseSettings> create_face_recognition_node_settings(const ObjectPtrJSON&);

std::unique_ptr<task::IAbstractTask> create_face_recognition_node(const std::shared_ptr<task::BaseSettings>& settings);

}  // namespace step::proc