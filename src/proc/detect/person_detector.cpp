#include "registrator.hpp"

#include <core/task/settings_factory.hpp>
#include <core/task/task_factory.hpp>

#include <video/frame/utils/frame_utils_opencv.hpp>

#include <proc/settings/settings_person_detector.hpp>
#include <proc/settings/settings_resizer.hpp>

#include <proc/interfaces/effect_interface.hpp>
#include <proc/interfaces/neural_net_interface.hpp>

#include <proc/neural/yolo/yolox_wrapper.hpp>

namespace step::proc {

class PersonDetector : public BaseDetector<SettingsPersonDetector>
{
public:
    PersonDetector(const std::shared_ptr<task::BaseSettings>& settings)
    {
        set_settings(*settings);

        auto resizer_settings = CREATE_SETTINGS(m_typed_settings.get_resizer_cfg());
        m_resizer = IEffect::from_abstract(CREATE_TASK_UNIQUE(resizer_settings));

        m_net = INeuralNet::from_abstract(CREATE_TASK_UNIQUE(CREATE_SETTINGS(m_typed_settings.get_neural_net_cfg())));

        const auto net_output_shape = m_net->get_output_shape();

        YoloxWrapper::Initializer yolo_init;
        yolo_init.frame_size = dynamic_cast<SettingsResizer*>(resizer_settings.get())->get_frame_size();
        yolo_init.grid_count = net_output_shape[1];
        yolo_init.class_count = net_output_shape[2] - 5;
        yolo_init.nms_threshold = 0.7;
        yolo_init.prob_threshold = 0.4;
        yolo_init.strides = {8, 16, 32};

        m_yolox.initialize(std::move(yolo_init));
    }

    DetectionResult process(video::Frame& frame)
    {
        //utils::ExecutionTimer<Milliseconds> timer("PersonDetector");
        auto resized = m_resizer->process(frame);
        auto neural_output = m_net->process(resized);
        auto yolo_objects = m_yolox.process(neural_output, frame.size);

        std::vector<Rect> bboxes;
        std::transform(yolo_objects.cbegin(), yolo_objects.cend(), std::back_inserter(bboxes), [](const auto& item) {
            return step::Rect(static_cast<int>(item.rect.x), static_cast<int>(item.rect.y),
                              static_cast<int>(item.rect.x + item.rect.width),
                              static_cast<int>(item.rect.y + item.rect.height));
        });

        DetectionResult result(std::move(bboxes));
        return result;
    }

private:
    std::unique_ptr<IEffect> m_resizer;
    std::unique_ptr<INeuralNet> m_net;
    YoloxWrapper m_yolox;
};

std::unique_ptr<IDetector> create_person_detector(const std::shared_ptr<task::BaseSettings>& settings)
{
    STEP_ASSERT(settings, "Can't create person detector: empty settings!");

    return std::make_unique<PersonDetector>(settings);
}

}  // namespace step::proc