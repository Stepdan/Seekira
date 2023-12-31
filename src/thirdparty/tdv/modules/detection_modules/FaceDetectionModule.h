#ifndef FACEDETECTOR_H
#define FACEDETECTOR_H

//#include <tdv/modules/DetectionModules/BaseDetectionModule.h>
#include "BaseDetectionModule.h"

namespace tdv {

namespace modules {

class FaceDetectionModule : public BaseDetection::BaseDetectionModule<FaceDetectionModule>
{
    friend class ONNXModule<FaceDetectionModule>;

public:
    FaceDetectionModule(const tdv::data::Context& config);
    static const std::string CLASS_NAME;
};

}  // namespace modules
}  // namespace tdv

#endif  // FACEDETECTOR_H
