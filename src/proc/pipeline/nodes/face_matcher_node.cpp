#include "face_matcher_node.hpp"

#include <core/base/types/config_fields.hpp>
#include <core/task/settings_factory.hpp>
#include <core/task/task_factory.hpp>

#include <proc/interfaces/detector_interface.hpp>
#include <proc/interfaces/face_engine_user.hpp>

#include <proc/face_engine/holder/person_holder.hpp>

namespace step::proc {

const std::string FaceMatcherNodeSettings::SETTINGS_ID = "FaceMatcherNodeSettings";

std::shared_ptr<task::BaseSettings> create_face_matcher_node_settings(const ObjectPtrJSON& cfg)
{
    return std::make_shared<FaceMatcherNodeSettings>(cfg);
}

void FaceMatcherNodeSettings::deserialize(const ObjectPtrJSON& container)
{
    auto skip_flag_opt = json::get_opt<bool>(container, CFG_FLD::SKIP_FLAG);
    if (skip_flag_opt.has_value())
        m_skip_flag = skip_flag_opt.value();

    auto face_engine_conn_id_opt = json::get_opt<std::string>(container, CFG_FLD::FACE_ENGINE_CONNECTION_ID);
    if (face_engine_conn_id_opt.has_value())
    {
        m_face_engine_conn_id = face_engine_conn_id_opt.value();
        STEP_ASSERT(!m_face_engine_conn_id.empty(), "FACE_ENGINE_CONNECTION_ID can't be empty!");
    }

    auto persons_pathes_json = json::opt_array(container, CFG_FLD::PERSON_HOLDER_PATHES);
    if (persons_pathes_json)
    {
        json::for_each_in_array<std::string>(
            persons_pathes_json, [this](const std::string& path) { m_person_holder_pathes.push_back(path); });
    }
}

}  // namespace step::proc

namespace step::proc {

class FaceMatcherPipelineNode : public PipelineNodeTask<video::Frame, FaceMatcherNodeSettings>, public IFaceEngineUser
{
public:
    FaceMatcherPipelineNode(const std::shared_ptr<task::BaseSettings>& settings)
    {
        set_settings(*settings);

        const auto conn_id = m_typed_settings.get_face_engine_conn_id();
        set_conn_id(conn_id);
        Connector::connect(this);

        for (const auto& path : m_typed_settings.get_person_holder_pathes())
        {
            m_persons.push_back(PersonHolder(conn_id));
            m_persons.back().load_from_dir(path);
        }
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

        Faces matched_faces;
        for (const auto& face : faces_opt.value())
        {
            for (const auto& person : m_persons)
            {
                if (person.compare(face))
                {
                    face->set_matched(true);
                    matched_faces.push_back(face);
                    break;
                }
            }
        }

        if (!matched_faces.empty())
        {
            pipeline_data->storage.set_attachment(CFG_FLD::FACE_MATCHING_RESULT,
                                                  std::make_any<decltype(matched_faces)>(matched_faces));
        }
    }

private:
    std::vector<PersonHolder> m_persons;
};

std::unique_ptr<task::IAbstractTask> create_face_matcher_node(const std::shared_ptr<task::BaseSettings>& settings)
{
    return std::make_unique<FaceMatcherPipelineNode>(settings);
}

}  // namespace step::proc