#pragma once

#include <base/interfaces/serializable.hpp>

#include <base/utils/string_utils.hpp>

#include <fmt/format.h>

namespace step::pipeline {

enum class SyncPolicy
{
    Undefined,
    Sync,
    ParallelWait,
    ParallelNoWait,
};

struct PipelineSettings : public ISerializable
{
    std::string name;
    SyncPolicy sync_policy{SyncPolicy::Undefined};

    PipelineSettings() = default;
    PipelineSettings(const ObjectPtrJSON& config);

    void deserialize(const ObjectPtrJSON& config);
};

}  // namespace step::pipeline

template <>
struct fmt::formatter<step::pipeline::PipelineSettings> : formatter<string_view>
{
    template <typename FormatContext>
    auto format(const step::pipeline::PipelineSettings& settings, FormatContext& ctx)
    {
        return format_to(ctx.out(), "name: {}; sync_mode {};", settings.name,
                         step::utils::to_string(settings.sync_policy));
    }
};
