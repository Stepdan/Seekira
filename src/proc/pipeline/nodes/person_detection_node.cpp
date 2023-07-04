#include "person_detection_node.hpp"

#include <core/base/types/config_fields.hpp>
#include <core/task/settings_factory.hpp>
#include <core/task/task_factory.hpp>

#include <proc/interfaces/detector_interface.hpp>

namespace step::proc {

const std::string PersonDetectionNodeSettings::SETTINGS_ID = "PersonDetectionNodeSettings";

std::shared_ptr<task::BaseSettings> create_person_detection_node_settings(const ObjectPtrJSON& cfg)
{
    return std::make_shared<PersonDetectionNodeSettings>(cfg);
}

void PersonDetectionNodeSettings::deserialize(const ObjectPtrJSON& container)
{
    auto settings_json = json::get_object(container, CFG_FLD::SETTINGS);
    m_person_detector_settings = CREATE_SETTINGS(settings_json);
}

}  // namespace step::proc

namespace step::proc {

class PersonDetectionPipelineNode : public PipelineNodeTask<video::Frame, PersonDetectionNodeSettings>
{
public:
    PersonDetectionPipelineNode(const std::shared_ptr<task::BaseSettings>& settings)
    {
        set_settings(*settings);

        auto person_detector_settings_base = m_typed_settings.get_person_detector_settings_base();
        STEP_ASSERT(person_detector_settings_base, "Invalid person_detector_settings_base");
        m_person_detector = IDetector::from_abstract(CREATE_TASK_UNIQUE(person_detector_settings_base));
    }

    void process(PipelineDataPtr<video::Frame> pipeline_data) override
    {
        auto detect_result = m_person_detector->process(pipeline_data->data);
    }

private:
    std::unique_ptr<IDetector> m_person_detector;
};

std::unique_ptr<task::IAbstractTask> create_person_detection_node(const std::shared_ptr<task::BaseSettings>& settings)
{
    return std::make_unique<PersonDetectionPipelineNode>(settings);
}

}  // namespace step::proc