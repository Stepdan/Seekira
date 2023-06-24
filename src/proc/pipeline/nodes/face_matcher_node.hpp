#pragma once

#include <core/log/log.hpp>

#include <proc/pipeline/pipeline_task.hpp>

#include <filesystem>

namespace step::proc {

class FaceMatcherNodeSettings : public task::BaseSettings
{
public:
    TASK_SETTINGS(FaceMatcherNodeSettings)

    FaceMatcherNodeSettings() = default;

    bool operator==(const FaceMatcherNodeSettings& rhs) const noexcept { return false; }
    bool operator!=(const FaceMatcherNodeSettings& rhs) const noexcept { return !(*this == rhs); }

    void set_face_engine_conn_id(const std::string& value) { m_face_engine_conn_id = value; }
    const std::string& get_face_engine_conn_id() const noexcept { return m_face_engine_conn_id; }

    void set_skip_flag(bool value) { m_skip_flag = true; }
    bool get_skip_flag() const noexcept { return m_skip_flag; }

    void set_person_holder_pathes(const std::vector<std::filesystem::path>& pathes) { m_person_holder_pathes = pathes; }
    const std::vector<std::filesystem::path>& get_person_holder_pathes() const noexcept
    {
        return m_person_holder_pathes;
    }

private:
    std::string m_face_engine_conn_id;
    bool m_skip_flag{false};
    std::vector<std::filesystem::path> m_person_holder_pathes;
};

std::shared_ptr<task::BaseSettings> create_face_matcher_node_settings(const ObjectPtrJSON&);

std::unique_ptr<task::IAbstractTask> create_face_matcher_node(const std::shared_ptr<task::BaseSettings>& settings);

}  // namespace step::proc