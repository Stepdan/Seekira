#include "registrator.hpp"

#include <core/log/log.hpp>

namespace step::proc {

class EffectEmpty : public BaseEffect<SettingsEmpty>
{
public:
    EffectEmpty(const std::shared_ptr<task::BaseSettings>& settings) { set_settings(*settings); }

    video::Frame process(EffectMultiInput input) override
    {
        STEP_LOG(L_INFO, "EffectEmpty process");
        return video::Frame();
    }
};

std::unique_ptr<IEffect> create_effect_empty(const std::shared_ptr<task::BaseSettings>& settings)
{
    return std::make_unique<EffectEmpty>(settings);
}

}  // namespace step::proc