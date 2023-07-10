#pragma once

#include <core/task/base_task.hpp>

#include <video/frame/interfaces/frame.hpp>

namespace step::proc {

using IEffect = task::ITask<video::Frame&, video::Frame>;

template <typename TSettings>
using BaseEffect = task::BaseTask<TSettings, video::Frame&, video::Frame>;

}  // namespace step::proc