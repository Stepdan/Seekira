#include "face_engine_factory.hpp"

#include <core/exception/assert.hpp>

#include <proc/face_engine/impl/tdv/face_engine_tdv.hpp>

namespace step::proc {

std::shared_ptr<IFaceEngine> create_face_engine(IFaceEngine::Initializer&& init)
{
    switch (init.type)
    {
        case FaceEngineType::TDV:
            return create_face_engine_tdv(std::move(init));
        default:
            STEP_UNDEFINED("Invalid FaceEngineType!");
    }

    return nullptr;
}

}  // namespace step::proc