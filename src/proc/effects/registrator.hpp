#pragma once

#include <proc/interfaces/effect_interface.hpp>

namespace step::proc {

std::unique_ptr<IEffect> create_effect_empty(const std::shared_ptr<task::BaseSettings>& settings);
std::unique_ptr<IEffect> create_effect_resizer(const std::shared_ptr<task::BaseSettings>& settings);

}  // namespace step::proc