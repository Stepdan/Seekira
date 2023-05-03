#pragma once

#include <pipeline/pipeline_task.hpp>

#include <base/utils/log_utils.hpp>

namespace step::pipeline {

class InputNodeSettings : public task::BaseSettings
{
public:
    TASK_SETTINGS(InputNodeSettings)

    InputNodeSettings() = default;

    bool operator==(const InputNodeSettings& rhs) const noexcept { return true; }
    bool operator!=(const InputNodeSettings& rhs) const noexcept { return !(*this == rhs); }
};

std::shared_ptr<task::BaseSettings> create_input_node_settings(const ObjectPtrJSON& cfg);

template <typename TData>
class InputPipelineNode : public PipelineNodeTask<TData, InputNodeSettings>
{
    using DataType = PipelineNodeTask<TData, InputNodeSettings>::DataType;

public:
    InputPipelineNode() = default;

    void process(DataType) override { utils::log_info_msg("Processing InputPipelineNode"); }
};

template <typename TData>
std::unique_ptr<task::ITask<std::shared_ptr<PipelineData<TData>>>> create_input_node(
    const std::shared_ptr<task::BaseSettings>& settings)
{
    //const InputNodeSettings& typed_settings = dynamic_cast<const InputNodeSettings&>(settings);
    std::unique_ptr<task::ITask<std::shared_ptr<PipelineData<TData>>>> node =
        std::make_unique<InputPipelineNode<TData>>();

    return node;
}

}  // namespace step::pipeline