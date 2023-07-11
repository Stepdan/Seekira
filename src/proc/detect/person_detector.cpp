#include "registrator.hpp"

#include <video/frame/utils/frame_utils_opencv.hpp>

#include <proc/settings/settings_person_detector.hpp>

#include <proc/interfaces/effect_interface.hpp>
#include <proc/interfaces/neural_net_interface.hpp>

#include <core/task/settings_factory.hpp>
#include <core/task/task_factory.hpp>

#include <opencv2/dnn.hpp>

namespace step::proc {

class PersonDetector : public BaseDetector<SettingsPersonDetector>
{
public:
    PersonDetector(const std::shared_ptr<task::BaseSettings>& settings)
    {
        set_settings(*settings);
        m_resizer = IEffect::from_abstract(CREATE_TASK_UNIQUE(CREATE_SETTINGS(m_typed_settings.get_resizer_cfg())));
        m_net = INeuralNet::from_abstract(CREATE_TASK_UNIQUE(CREATE_SETTINGS(m_typed_settings.get_neural_net_cfg())));
    }

    DetectionResult process(video::Frame& frame)
    {
        {
            utils::ExecutionTimer<Milliseconds> timer("PersonDetector");
            auto resized = m_resizer->process(frame);
            m_net->process(resized);
        }
        return {};
    }

private:
    std::unique_ptr<IEffect> m_resizer;
    std::unique_ptr<INeuralNet> m_net;
};

std::unique_ptr<IDetector> create_person_detector(const std::shared_ptr<task::BaseSettings>& settings)
{
    STEP_ASSERT(settings, "Can't create person detector: empty settings!");

    return std::make_unique<PersonDetector>(settings);
}

}  // namespace step::proc