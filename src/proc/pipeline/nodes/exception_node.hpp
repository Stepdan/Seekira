#pragma once

#include <proc/pipeline/pipeline_task.hpp>

#include <core/exception/assert.hpp>

namespace step::proc {

class ExceptionNodeSettings : public task::BaseSettings
{
public:
    TASK_SETTINGS(ExceptionNodeSettings)

    ExceptionNodeSettings() = default;

    bool operator==(const ExceptionNodeSettings& rhs) const noexcept { return false; }
    bool operator!=(const ExceptionNodeSettings& rhs) const noexcept { return !(*this == rhs); }
};

std::shared_ptr<task::BaseSettings> create_exception_node_settings(const ObjectPtrJSON&);

template <typename TData>
class ExceptionPipelineNode : public PipelineNodeTask<TData, ExceptionNodeSettings>
{
    using DataType = PipelineNodeTask<TData, ExceptionNodeSettings>::DataType;

public:
    ExceptionPipelineNode() = default;

    void process(DataType) override { STEP_THROW_RUNTIME("Exception throw by ExceptionPipelineNode"); }
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

}  // namespace step::proc