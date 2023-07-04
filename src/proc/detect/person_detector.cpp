#include "registrator.hpp"

#include <video/frame/utils/frame_utils_opencv.hpp>

#include <proc/settings/settings_person_detector.hpp>

#include <opencv2/dnn/dnn.hpp>

namespace step::proc {

class PersonDetector : public BaseDetector<SettingsPersonDetector>
{
public:
    PersonDetector(const std::shared_ptr<task::BaseSettings>& settings)
    {
        set_settings(*settings);
        m_net = cv::dnn::readNetFromONNX(m_typed_settings.get_model_path());
        m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    }

    DetectionResult process(video::Frame& frame)
    {
        auto mat = video::utils::to_mat_deep(frame);
        bool swap_rb = true;
        cv::Mat blob = cv::dnn::blobFromImage(mat, 1 / 255.0, cv::Size(frame.size.width, frame.size.height),
                                              cv::Scalar(0, 0, 0), swap_rb, false);
        m_net.setInput(blob);
        auto result = m_net.forward("output");
        return {};
    }

private:
    cv::dnn::Net m_net;
};

std::unique_ptr<IDetector> create_person_detector(const std::shared_ptr<task::BaseSettings>& settings)
{
    STEP_ASSERT(settings, "Can't create person detector: empty settings!");

    return std::make_unique<PersonDetector>(settings);
}

}  // namespace step::proc