#pragma once

#include <video/frame/interfaces/frame.hpp>

#include <proc/interfaces/face.hpp>

namespace step::proc {

void draw(video::Frame& frame, const FacePtr& obj);

}