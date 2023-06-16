#pragma once

#include <core/base/interfaces/serializable.hpp>

#include <core/base/utils/string_utils.hpp>

#include <fmt/format.h>

namespace step::proc {

enum class PipelineSyncPolicy
{
    Undefined,
    Sync,
    ParallelWait,
    ParallelNoWait,
};

struct PipelineSettings : public ISerializable
{
    std::string name;
    PipelineSyncPolicy sync_policy{PipelineSyncPolicy::Undefined};

    PipelineSettings() = default;
    PipelineSettings(const ObjectPtrJSON& config);

    void deserialize(const ObjectPtrJSON& config);
};

}  // namespace step::proc

template <>
struct fmt::formatter<step::proc::PipelineSettings> : fmt::formatter<string_view>
{
    template <typename FormatContext>
    auto format(const step::proc::PipelineSettings& settings, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "name: {}; sync_mode {};", settings.name,
                              step::utils::to_string(settings.sync_policy));
    }
};
