#pragma once

#include <core/task/base_task.hpp>

#include <video/frame/interfaces/frame.hpp>

#include <vector>

namespace step::proc {

using NeuralOutput = std::vector<float>;

using INeuralNet = task::ITask<video::Frame&, NeuralOutput>;

template <typename TSettings>
using BaseNeuralNet = task::BaseTask<TSettings, video::Frame&, NeuralOutput>;

}  // namespace step::proc