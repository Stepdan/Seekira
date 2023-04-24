#pragma once

#include <base/interfaces/serializable.hpp>

#include <base/utils/string_utils.hpp>

#include <fmt/format.h>

namespace step::video::pipeline {

enum class FramePipelineSyncPolicy
{
    Undefined,
    Sync,            // Sync mode
    ParallelWait,    // Parallel with waiting all threads finishing
    ParallelNoWait,  // Parallel executing without waiting (skip data).
};

class FramePipelineSettings : public ISerializable
{
public:
    FramePipelineSettings() = default;
    FramePipelineSettings(const ObjectPtrJSON& config);

    void deserialize(const ObjectPtrJSON& config);

public:
    const std::string& get_pipeline_name() const noexcept { return m_name; }
    FramePipelineSyncPolicy get_sync_policy() const noexcept { return m_sync_policy; }

private:
    std::string m_name;
    FramePipelineSyncPolicy m_sync_policy{FramePipelineSyncPolicy::Undefined};
};

}  // namespace step::video::pipeline

template <>
struct fmt::formatter<step::video::pipeline::FramePipelineSyncPolicy> : formatter<string_view>
{
    template <typename FormatContext>
    auto format(const step::video::pipeline::FramePipelineSyncPolicy& mode, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", step::utils::to_string(mode));
    }
};

template <>
struct fmt::formatter<step::video::pipeline::FramePipelineSettings> : formatter<string_view>
{
    template <typename FormatContext>
    auto format(const step::video::pipeline::FramePipelineSettings& settings, FormatContext& ctx)
    {
        return format_to(ctx.out(), "name: {}; sync mode: {}", settings.get_pipeline_name(),
                         step::utils::to_string(settings.get_sync_policy()));
    }
};
