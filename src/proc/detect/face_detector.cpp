#include "registrator.hpp"

#include <core/log/log.hpp>
#include <core/base/types/config_fields.hpp>

#include <proc/interfaces/face_engine_user.hpp>
#include <proc/face_engine/face_engine_factory.hpp>
#include <proc/settings/settings_face_detector.hpp>

namespace step::proc {

class FaceDetector : public BaseDetector<SettingsFaceDetector>, public IFaceEngineUser
{
public:
    FaceDetector(const std::shared_ptr<task::BaseSettings>& settings)
    {
        set_settings(*settings);

        const auto conn_id = m_typed_settings.get_face_engine_conn_id();
        if (conn_id.empty())
        {
            init_face_engine_internal();
        }
        else
        {
            set_conn_id(conn_id);
            Connector::connect(this);
        }
    }

    DetectionResult process(video::Frame& frame)
    {
        auto faces = get_face_engine_impl()->detect(frame);

        std::vector<Rect> bboxes;
        bboxes.reserve(faces.size());
        for (const auto& face : faces)
        {
            STEP_ASSERT(face, "Invalid face!");
            bboxes.push_back(face->get_rect());
        }

        MetaStorage storage;
        storage.set_attachment(CFG_FLD::FACES, std::move(faces));

        return {std::move(bboxes), std::move(storage)};
    }

private:
    void init_face_engine_internal()
    {
        m_face_engine_internal = create_face_engine(m_typed_settings.get_face_engine_init());
    }

    std::shared_ptr<IFaceEngine> get_face_engine_impl()
    {
        if (m_face_engine_internal)
            return m_face_engine_internal;

        return get_face_engine(true);
    }

private:
    std::shared_ptr<IFaceEngine> m_face_engine_internal{nullptr};
};

}  // namespace step::proc

namespace step::proc {

std::unique_ptr<IDetector> create_face_detector(const std::shared_ptr<task::BaseSettings>& settings)
{
    STEP_ASSERT(settings, "Can't create face detector: empty settings!");

    return std::make_unique<FaceDetector>(settings);
}

}  // namespace step::proc