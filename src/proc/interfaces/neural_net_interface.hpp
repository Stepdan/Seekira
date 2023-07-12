#pragma once

#include <core/task/base_task.hpp>

#include <video/frame/interfaces/frame.hpp>

#include <vector>

namespace step::proc {

struct NeuralOutput
{
    std::vector<float> data_vec;
    const float* data_ptr{nullptr};
};

class INeuralNetExt
{
public:
    virtual ~INeuralNetExt() = default;

    virtual std::vector<int64_t> get_input_shape() const = 0;
    virtual std::vector<int64_t> get_output_shape() const = 0;
};

using INeuralNet = task::ITask<video::Frame&, NeuralOutput, INeuralNetExt>;

template <typename TSettings>
using BaseNeuralNet = task::BaseTask<TSettings, video::Frame&, NeuralOutput, INeuralNetExt>;

}  // namespace step::proc