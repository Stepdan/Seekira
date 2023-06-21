#pragma once

#include <proc/interfaces/face_engine.hpp>

#include <memory>

namespace step::proc {

std::shared_ptr<IFaceEngine> create_face_engine_tdv(IFaceEngine::Initializer&&);

}