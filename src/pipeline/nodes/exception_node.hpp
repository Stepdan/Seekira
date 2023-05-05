#pragma once

#include <pipeline/pipeline_task.hpp>

#include <base/utils/exception/assert.hpp>

namespace step::pipeline {

class ExceptionNodeSettings : public task::BaseSettings
{
public:
    TASK_SETTINGS(ExceptionNodeSettings)

    ExceptionNodeSettings() = default;

    bool operator==(const ExceptionNodeSettings& rhs) const noexcept { return true; }
    bool operator!=(const ExceptionNodeSettings& rhs) const noexcept { return !(*this == rhs); }
};

std::shared_ptr<task::BaseSettings> create_exception_node_settings(const ObjectPtrJSON&);

template <typename TData>
class ExceptionPipelineNode : public PipelineNodeTask<TData, ExceptionNodeSettings>
{
    using DataType = PipelineNodeTask<TData, ExceptionNodeSettings>::DataType;

public:
    ExceptionPipelineNode() = default;

    void process(DataType) override { STEP_ASSERT(false, "Exception throw by ExceptionPipelineNode"); }
};

template <typename TData>
std::unique_ptr<task::ITask<std::shared_ptr<PipelineData<TData>>>> create_exception_node(
    const std::shared_ptr<task::BaseSettings>& settings)
{
    //const ExceptionNodeSettings& typed_settings = dynamic_cast<const ExceptionNodeSettings&>(settings);
    std::unique_ptr<task::ITask<std::shared_ptr<PipelineData<TData>>>> node =
        std::make_unique<ExceptionPipelineNode<TData>>();

    return node;
}

}  // namespace step::pipeline