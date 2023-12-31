#include "pipeline_settings.hpp"

#include <core/base/types/config_fields.hpp>
#include <core/base/utils/find_pair.hpp>

namespace {

/* clang-format off */
const std::pair<step::proc::PipelineSyncPolicy, std::string> g_sync_modes[] = {
    { step::proc::PipelineSyncPolicy::ParallelNoWait  , "parallel_no_wait" },
    { step::proc::PipelineSyncPolicy::ParallelWait    , "parallel_wait"    },
    { step::proc::PipelineSyncPolicy::Sync            , "sync"             },
};
/* clang-format on */

}  // namespace

// to_string
namespace step::utils {

template <>
std::string to_string(step::proc::PipelineSyncPolicy mode)
{
    return find_by_type(mode, g_sync_modes);
}

template <>
void from_string(step::proc::PipelineSyncPolicy& mode, const std::string& str)
{
    find_by_str(str, mode, g_sync_modes);
}

}  // namespace step::utils

namespace step::proc {

PipelineSettings::PipelineSettings(const ObjectPtrJSON& config) { deserialize(config); }

void PipelineSettings::deserialize(const ObjectPtrJSON& config)
{
    name = json::get<std::string>(config, CFG_FLD::NAME);
    utils::from_string(sync_policy, json::get<std::string>(config, CFG_FLD::SYNC_MODE));
}

}  // namespace step::proc