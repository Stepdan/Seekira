#pragma once

#include <core/log/log.hpp>

#include <proc/face_engine/holder/person_holder.hpp>

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

    void set_person_holder_initializers(const std::vector<PersonHolder::Initializer>& inits)
    {
        m_person_holder_initializers = inits;
    }
    const std::vector<PersonHolder::Initializer>& get_person_holder_initializers() const noexcept
    {
        return m_person_holder_initializers;
    }

private:
    std::string m_face_engine_conn_id;
    bool m_skip_flag{false};
    std::vector<PersonHolder::Initializer> m_person_holder_initializers;
};

std::shared_ptr<task::BaseSettings> create_face_matcher_node_settings(const ObjectPtrJSON&);

std::unique_ptr<task::IAbstractTask> create_face_matcher_node(const std::shared_ptr<task::BaseSettings>& settings);

}  // namespace step::proc