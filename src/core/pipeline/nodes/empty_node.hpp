#pragma once

#include <core/pipeline/pipeline_task.hpp>

#include <core/log/log.hpp>

namespace step::pipeline {

class EmptyNodeSettings : public task::BaseSettings
{
public:
    TASK_SETTINGS(EmptyNodeSettings)

    EmptyNodeSettings() = default;

    bool operator==(const EmptyNodeSettings& rhs) const noexcept { return true; }
    bool operator!=(const EmptyNodeSettings& rhs) const noexcept { return !(*this == rhs); }
};

std::shared_ptr<task::BaseSettings> create_empty_node_settings(const ObjectPtrJSON&);

template <typename TData>
class EmptyPipelineNode : public PipelineNodeTask<TData, EmptyNodeSettings>
{
    using DataType = PipelineNodeTask<TData, EmptyNodeSettings>::DataType;

public:
    EmptyPipelineNode() = default;

    void process(DataType) override { STEP_LOG(L_INFO, "Processing EmptyPipelineNode"); }
};

template <typename TData>
std::unique_ptr<task::ITask<std::shared_ptr<PipelineData<TData>>>> create_empty_node(
    const std::shared_ptr<task::BaseSettings>& settings)
{
    //const EmptyNodeSettings& typed_settings = dynamic_cast<const EmptyNodeSettings&>(settings);
    std::unique_ptr<task::ITask<std::shared_ptr<PipelineData<TData>>>> node =
        std::make_unique<EmptyPipelineNode<TData>>();

    return node;
}

}  // namespace step::pipeline