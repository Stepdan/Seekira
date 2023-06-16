#include "face_detection_node.hpp"

#include <core/base/types/config_fields.hpp>
#include <core/task/settings_factory.hpp>
#include <core/task/task_factory.hpp>

#include <proc/interfaces/detector_interface.hpp>

namespace step::proc {

const std::string FaceDetectionNodeSettings::SETTINGS_ID = "FaceDetectionNodeSettings";

std::shared_ptr<task::BaseSettings> create_face_detection_node_settings(const ObjectPtrJSON& cfg)
{
    return std::make_shared<FaceDetectionNodeSettings>(cfg);
}

void FaceDetectionNodeSettings::deserialize(const ObjectPtrJSON& container)
{
    auto face_detector_settings_json = json::get_object(container, CFG_FLD::FACE_DETECTOR_SETTINGS);
    m_face_detector_settings = CREATE_SETTINGS(face_detector_settings_json);
}

}  // namespace step::proc

namespace step::proc {

class FaceDetectionPipelineNode : public PipelineNodeTask<video::Frame, FaceDetectionNodeSettings>
{
public:
    FaceDetectionPipelineNode(const std::shared_ptr<task::BaseSettings>& settings)
    {
        set_settings(*settings);

        auto face_detector_settings_base = m_typed_settings.get_face_detector_settings_base();
        STEP_ASSERT(face_detector_settings_base, "Invalid face_detector_settings_base");
        m_face_detector = IDetector::from_abstract(CREATE_TASK_UNIQUE(face_detector_settings_base));
    }

    void process(PipelineDataPtr<video::Frame> pipeline_data) override
    {
        auto detect_result = m_face_detector->process(pipeline_data->data);

        pipeline_data->storage.set_attachment(CFG_FLD::FACE_DETECTION_RESULT,
                                              std::make_any<decltype(detect_result)>(detect_result));
    }

private:
    std::unique_ptr<IDetector> m_face_detector;
};

std::unique_ptr<task::IAbstractTask> create_face_detection_node(const std::shared_ptr<task::BaseSettings>& settings)
{
    return std::make_unique<FaceDetectionPipelineNode>(settings);
}

}  // namespace step::proc