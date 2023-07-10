#pragma once

#include <proc/interfaces/neural_net_interface.hpp>

namespace step::proc {

std::unique_ptr<INeuralNet> create_onnxruntime_neural_net(const std::shared_ptr<task::BaseSettings>& settings);

}  // namespace step::proc