#include "face_detector_factory.hpp"
#include "registrator.hpp"

#include <core/exception/assert.hpp>

namespace step::proc {

std::unique_ptr<IDetector> create_face_detector(const std::shared_ptr<task::BaseSettings>& settings)
{
    STEP_ASSERT(settings, "Can't create face detector: empty settings!");

    // TODO bad cast
    const SettingsFaceDetector& typed_settings = dynamic_cast<const SettingsFaceDetector&>(*settings);

    switch (typed_settings.get_algorithm())
    {
        case SettingsFaceDetector::Algorithm::TDV:
            return create_face_detector_tdv(settings);
            break;
        default:
            STEP_UNDEFINED("Unknown face detector algorithm type!");
    }

    return nullptr;
}

}  // namespace step::proc
