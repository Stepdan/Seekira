#include "registrator.hpp"

#include <proc/settings/settings_tracker_kcf.hpp>

namespace step::proc {

class TrackerKCF : public BaseTracker<SettingsTrackerKCF>
{
public:
    TrackerKCF(const std::shared_ptr<task::BaseSettings>& settings) { set_settings(*settings); }

    void process(video::Frame& frame) {}
};

TrackerPtr create_tracker_kcf_opencv(const std::shared_ptr<task::BaseSettings>& settings)
{
    return std::make_unique<TrackerKCF>(settings);
}

}  // namespace step::proc