#include "registrator.hpp"

#include <core/log/log.hpp>

namespace step::proc {

class FaceDetector : public BaseDetector<SettingsFaceDetector>
{
public:
    FaceDetector(const std::shared_ptr<task::BaseSettings>& settings) { set_settings(*settings); }

    DetectionResult process(video::Frame frame) override
    {
        STEP_LOG(L_INFO, "FaceDetector process");
        return DetectionResult();
    }
};

std::unique_ptr<IDetector> create_face_detector(const std::shared_ptr<task::BaseSettings>& settings)
{
    return std::make_unique<FaceDetector>(settings);
}

}  // namespace step::proc