#pragma once

#include <video/frame/interfaces/frame.hpp>

namespace step::proc {

template <typename TObj>
void draw(video::Frame& frame, const TObj& obj);

}