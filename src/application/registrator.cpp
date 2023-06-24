#include "registrator.hpp"

#include <core/task/settings_factory.hpp>
#include <core/task/task_factory.hpp>

#include <video/frame/interfaces/frame.hpp>

#include <proc/detect/registrator.hpp>
#include <proc/effects/registrator.hpp>

#include <proc/pipeline/nodes/empty_node.hpp>
#include <proc/pipeline/nodes/exception_node.hpp>
#include <proc/pipeline/nodes/input_node.hpp>
#include <proc/pipeline/nodes/face_detection_node.hpp>
#include <proc/pipeline/nodes/face_recognition_node.hpp>
#include <proc/pipeline/nodes/resizer_node.hpp>
#include <proc/pipeline/nodes/drawer_node.hpp>

#include <proc/settings/settings_face_detector.hpp>
#include <proc/settings/settings_resizer.hpp>
#include <proc/settings/settings_video_processor_task.hpp>

#include <proc/video/video_processor_task.hpp>

namespace step::app {

Registrator& Registrator::instance()
{
    static Registrator obj;
    return obj;
}

Registrator::Registrator()
{
    /* clang-format off */

    // Pipeline nodes settings
    REGISTER_TASK_SETTINGS_CREATOR(proc::InputNodeSettings          ::SETTINGS_ID, &proc::create_input_node_settings            );
    REGISTER_TASK_SETTINGS_CREATOR(proc::EmptyNodeSettings          ::SETTINGS_ID, &proc::create_empty_node_settings            );
    REGISTER_TASK_SETTINGS_CREATOR(proc::ExceptionNodeSettings      ::SETTINGS_ID, &proc::create_exception_node_settings        );
    REGISTER_TASK_SETTINGS_CREATOR(proc::FaceDetectionNodeSettings  ::SETTINGS_ID, &proc::create_face_detection_node_settings   );
    REGISTER_TASK_SETTINGS_CREATOR(proc::FaceRecognitionNodeSettings::SETTINGS_ID, &proc::create_face_recognition_node_settings );
    REGISTER_TASK_SETTINGS_CREATOR(proc::ResizerNodeSettings        ::SETTINGS_ID, &proc::create_resizer_node_settings          );
    REGISTER_TASK_SETTINGS_CREATOR(proc::DrawerNodeSettings         ::SETTINGS_ID, &proc::create_drawer_node_settings           );

    // Pipeline nodes tasks
    REGISTER_TASK_CREATOR_UNIQUE(proc::FaceDetectionNodeSettings    ::SETTINGS_ID, &proc::create_face_detection_node    );
    REGISTER_TASK_CREATOR_UNIQUE(proc::FaceRecognitionNodeSettings  ::SETTINGS_ID, &proc::create_face_recognition_node  );
    REGISTER_TASK_CREATOR_UNIQUE(proc::ResizerNodeSettings          ::SETTINGS_ID, &proc::create_resizer_node           );
    REGISTER_TASK_CREATOR_UNIQUE(proc::DrawerNodeSettings           ::SETTINGS_ID, &proc::create_drawer_node            );
    REGISTER_TASK_CREATOR_UNIQUE(proc::InputNodeSettings            ::SETTINGS_ID, &proc::create_input_node             <video::Frame>);
    REGISTER_TASK_CREATOR_UNIQUE(proc::EmptyNodeSettings            ::SETTINGS_ID, &proc::create_empty_node             <video::Frame>);
    REGISTER_TASK_CREATOR_UNIQUE(proc::ExceptionNodeSettings        ::SETTINGS_ID, &proc::create_exception_node         <video::Frame>);
    /* clang-format on */

    /* clang-format off */
    REGISTER_TASK_SETTINGS_CREATOR(proc::SettingsFaceDetector       ::SETTINGS_ID, &proc::create_face_detector_settings         );
    REGISTER_TASK_SETTINGS_CREATOR(proc::SettingsResizer            ::SETTINGS_ID, &proc::create_resizer_settings               );
    REGISTER_TASK_SETTINGS_CREATOR(proc::SettingsVideoProcessorTask ::SETTINGS_ID, &proc::create_video_processor_task_settings  );

    REGISTER_TASK_CREATOR_UNIQUE(proc::SettingsFaceDetector         ::SETTINGS_ID, &proc::create_face_detector          );
    REGISTER_TASK_CREATOR_UNIQUE(proc::SettingsResizer              ::SETTINGS_ID, &proc::create_effect_resizer         );
    REGISTER_TASK_CREATOR_UNIQUE(proc::SettingsVideoProcessorTask   ::SETTINGS_ID, &proc::create_video_processor_task   );
    /* clang-format on */
}

}  // namespace step::app