#include "face_recognition_node.hpp"

#include <core/base/types/config_fields.hpp>
#include <core/task/settings_factory.hpp>
#include <core/task/task_factory.hpp>

#include <proc/interfaces/detector_interface.hpp>
#include <proc/interfaces/face_engine_user.hpp>

namespace step::proc {

const std::string FaceRecognitionNodeSettings::SETTINGS_ID = "FaceRecognitionNodeSettings";

std::shared_ptr<task::BaseSettings> create_face_recognition_node_settings(const ObjectPtrJSON& cfg)
{
    return std::make_shared<FaceRecognitionNodeSettings>(cfg);
}

void FaceRecognitionNodeSettings::deserialize(const ObjectPtrJSON& container)
{
    auto skip_flag_opt = json::get_opt<bool>(container, CFG_FLD::SKIP_FLAG);
    if (skip_flag_opt.has_value())
        m_skip_flag = skip_flag_opt.value();

    auto face_engine_conn_id_opt = json::get_opt<std::string>(container, CFG_FLD::FACE_ENGINE_CONNECTION_ID);
    if (face_engine_conn_id_opt.has_value())
    {
        m_face_engine_conn_id = face_engine_conn_id_opt.value();
        STEP_ASSERT(!m_face_engine_conn_id.empty(), "FACE_ENGINE_CONNECTION_ID can't be empty!");
        return;
    }
}

}  // namespace step::proc

namespace step::proc {

class FaceRecognitionPipelineNode : public PipelineNodeTask<video::Frame, FaceRecognitionNodeSettings>,
                                    public IFaceEngineUser
{
public:
    FaceRecognitionPipelineNode(const std::shared_ptr<task::BaseSettings>& settings)
    {
        set_settings(*settings);

        const auto conn_id = m_typed_settings.get_face_engine_conn_id();
        set_conn_id(conn_id);
        Connector::connect(this);
    }

    void process(PipelineDataPtr<video::Frame> pipeline_data) override
    {
        if (m_typed_settings.get_skip_flag())
            return;

        const auto& face_detection_result_opt =
            pipeline_data->storage.get_attachment<DetectionResult>(CFG_FLD::FACE_DETECTION_RESULT);
        if (!face_detection_result_opt.has_value())
            return;

        const auto& faces_opt = face_detection_result_opt.value().data().get_attachment<Faces>(CFG_FLD::FACES);

        if (!faces_opt.has_value())
            return;

        for (const auto& face : faces_opt.value())
            get_face_engine(true)->recognize(face);
    }
};

std::unique_ptr<task::IAbstractTask> create_face_recognition_node(const std::shared_ptr<task::BaseSettings>& settings)
{
    return std::make_unique<FaceRecognitionPipelineNode>(settings);
}

}  // namespace step::proc