#pragma once

#include "pipeline_data.hpp"

#include <core/task/base_task.hpp>

namespace step::proc {

template <typename TData>
using IPipelineNodeTask = task::ITask<PipelineDataPtr<TData>>;

template <typename TData, typename TSettings>
using PipelineNodeTask = task::BaseTask<TSettings, PipelineDataPtr<TData>>;

}  // namespace step::proc