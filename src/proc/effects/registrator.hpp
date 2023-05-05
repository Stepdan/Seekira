#pragma once

#include <proc/interfaces/effect_interface.hpp>

#include <proc/settings/settings_empty.hpp>

namespace step::proc {

std::unique_ptr<IEffect> create_effect_empty(const std::shared_ptr<task::BaseSettings>& settings);

}