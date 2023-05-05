#pragma once

#include <abstract_task/base_task.hpp>

#include <video/frame/interfaces/frame.hpp>

namespace step::proc {

using EffectMultiInput = std::vector<video::Frame>;

using IEffect = task::ITask<EffectMultiInput, video::Frame>;

template <typename TSettings>
using BaseEffect = task::BaseTask<TSettings, EffectMultiInput, video::Frame>;

}  // namespace step::proc