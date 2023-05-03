#pragma once

#include <base/types/meta_storage.hpp>
#include <base/utils/exception/assert.hpp>
#include <abstract_task/base_task.hpp>

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
inline PipelineData<TData> clone_pipeline_data(const PipelineData<TData>& data)
{
    PipelineData<TData> copy;

    copy.data = data.data;
    copy.storage = data.storage;

    return copy;
}

template <typename TData>
inline std::shared_ptr<PipelineData<TData>> clone_pipeline_data_shared(
    const std::shared_ptr<PipelineData<TData>>& data_ptr)
{
    STEP_ASSERT(data_ptr, "Can't clone empty pipeline data!");
    return std::make_shared<PipelineData<TData>>(clone_pipeline_data(*data_ptr));
}

template <typename TData>
using IPipelineNodeTask = task::ITask<std::shared_ptr<PipelineData<TData>>>;

template <typename TData, typename TSettings>
using PipelineNodeTask = task::BaseTask<std::shared_ptr<PipelineData<TData>>, TSettings>;

}  // namespace step::pipeline