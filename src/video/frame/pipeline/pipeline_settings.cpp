#include "pipeline_settings.hpp"

#include <base/types/config_fields.hpp>
#include <base/utils/find_pair.hpp>
#include <base/utils/string_utils.hpp>

namespace {

/* clang-format off */

constexpr std::pair<step::video::pipeline::FramePipelineSyncPolicy, std::string_view> g_sync_modes[] = {
    { step::video::pipeline::FramePipelineSyncPolicy::ParallelNoWait  , "parallel_no_wait" },
    { step::video::pipeline::FramePipelineSyncPolicy::ParallelWait    , "parallel_wait"    },
    { step::video::pipeline::FramePipelineSyncPolicy::Sync            , "sync"             },
};

/* clang-format on */

}  // namespace

// to_string
namespace step::utils {

template <>
std::string to_string(video::pipeline::FramePipelineSyncPolicy mode)
{
    return find_by_type(mode, g_sync_modes);
}

template <>
void from_string(video::pipeline::FramePipelineSyncPolicy& mode, const std::string& str)
{
    find_by_str(str, mode, g_sync_modes);
}

}  // namespace step::utils

namespace step::video::pipeline {

FramePipelineSettings::FramePipelineSettings(const ObjectPtrJSON& config) { deserialize(config); }

void FramePipelineSettings::deserialize(const ObjectPtrJSON& config)
{
    m_name = json::get<std::string>(config, CFG_FLD::NAME);
    utils::from_string(m_sync_policy, json::get<std::string>(config, CFG_FLD::SYNC_MODE));
}

}  // namespace step::video::pipeline