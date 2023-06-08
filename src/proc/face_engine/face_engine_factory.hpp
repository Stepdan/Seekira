#pragma once

#include <proc/interfaces/face_engine.hpp>

#include <memory>

namespace step::proc {

std::unique_ptr<IFaceEngine> create_face_engine(IFaceEngine::Initializer&&);

}