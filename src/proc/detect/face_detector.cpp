#include "registrator.hpp"

#include <proc/face_engine/face_engine_factory.hpp>

#include <proc/settings/settings_face_detector.hpp>

namespace step::proc {

class FaceDetector : public BaseDetector<SettingsFaceDetector>
{
public:
    FaceDetector(const std::shared_ptr<task::BaseSettings>& settings)
    {
        set_settings(*settings);

        IFaceEngine::Initializer init;
        init.type = m_typed_settings.get_face_engine_type();
        init.models_path = m_typed_settings.get_model_path();
        init.mode = m_typed_settings.get_mode();

        m_face_engine = create_face_engine(std::move(init));
    }

    DetectionResult process(video::Frame& frame)
    {
        STEP_ASSERT(m_face_engine, "Invalid face engine!");

        auto faces = m_face_engine->detect(frame);

        std::vector<Rect> bboxes;
        bboxes.reserve(faces.size());
        for (const auto& face : faces)
        {
            STEP_ASSERT(face, "Invalid face!");
            bboxes.push_back(face->get_rect());
        }

        MetaStorage storage;
        storage.set_attachment("faces", std::move(faces));

        return {std::move(bboxes), std::move(storage)};
    }

private:
    std::unique_ptr<IFaceEngine> m_face_engine;
};

}  // namespace step::proc

namespace step::proc {

std::unique_ptr<IDetector> create_face_detector(const std::shared_ptr<task::BaseSettings>& settings)
{
    STEP_ASSERT(settings, "Can't create face detector: empty settings!");

    return std::make_unique<FaceDetector>(settings);
}

}  // namespace step::proc