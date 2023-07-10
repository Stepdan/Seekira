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

    void set_neural_net_cfg(const ObjectPtrJSON& value) { m_neural_net_cfg = value; }
    const ObjectPtrJSON& get_neural_net_cfg() const noexcept { return m_neural_net_cfg; }

    void set_resizer_cfg(const ObjectPtrJSON& value) { m_resizer_cfg = value; }
    const ObjectPtrJSON& get_resizer_cfg() const noexcept { return m_resizer_cfg; }

public:
    ObjectPtrJSON m_neural_net_cfg;
    ObjectPtrJSON m_resizer_cfg;
};

std::shared_ptr<task::BaseSettings> create_person_detector_settings(const ObjectPtrJSON&);

}  // namespace step::proc