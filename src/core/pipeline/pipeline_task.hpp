#pragma once

#include <core/exception/assert.hpp>
#include <core/base/types/meta_storage.hpp>
#include <core/task/base_task.hpp>

#include <memory>

namespace step::pipeline {

template <typename TData>
struct PipelineData
{
    TData data;
    MetaStorage storage;

    static std::shared_ptr<PipelineData<TData>> create(TData&& d)
    {
        return std::make_shared<PipelineData<TData>>(std::move(d), MetaStorage());
    }
};

template <typename TData>
using PipelineDataTypePtr = std::shared_ptr<PipelineData<TData>>;

template <typename TData>
inline PipelineData<TData> clone_pipeline_data(const PipelineData<TData>& data)
{
    PipelineData<TData> copy;

    copy.data = data.data;
    copy.storage = data.storage;

    return copy;
}

template <typename TData>
inline PipelineDataTypePtr<TData> clone_pipeline_data_shared(const PipelineDataTypePtr<TData>& data_ptr)
{
    STEP_ASSERT(data_ptr, "Can't clone empty pipeline data!");
    return std::make_shared<PipelineData<TData>>(clone_pipeline_data(*data_ptr));
}

template <typename TData>
using IPipelineNodeTask = task::ITask<PipelineDataTypePtr<TData>>;

template <typename TData, typename TSettings>
using PipelineNodeTask = task::BaseTask<TSettings, PipelineDataTypePtr<TData>>;

}  // namespace step::pipeline