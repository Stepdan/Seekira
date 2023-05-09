#pragma once

#include "pipeline_task.hpp"

#include <core/base/json/json_utils.hpp>
#include <core/task/settings_factory.hpp>
#include <core/task/task_factory.hpp>
#include <core/graph/node.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <core/log/log.hpp>

namespace step::pipeline {

using IdType = graph::BaseGraphNode::IdType;

template <typename TData>
class PipelineGraphNode : public graph::BaseGraphNode, public ISerializable
{
public:
    using Ptr = std::shared_ptr<PipelineGraphNode>;

public:
    PipelineGraphNode(const IdType& node_id, std::unique_ptr<IPipelineNodeTask<TData>>&& task) : m_task(std::move(task))
    {
        m_id = node_id;
    }

    PipelineGraphNode(const ObjectPtrJSON& config) { deserialize(config); }

    void process(const PipelineDataTypePtr<TData>& data)
    {
        STEP_LOG(L_INFO, "Processing pipeline node {}", m_id);
        m_task->process(data);
    }

private:
    void deserialize(const ObjectPtrJSON& container) override
    {
        m_id = json::get<IdType>(container, CFG_FLD::NODE);

        const auto task_settings = json::get_object(container, CFG_FLD::SETTINGS);
        auto base_task_settings = task::TaskSettingsFactory::instance().create(task_settings);

        m_task = task::TaskFactory<PipelineDataTypePtr<TData>>::instance().create_unique(base_task_settings);
    }

private:
    mutable std::unique_ptr<IPipelineNodeTask<TData>> m_task;
};

}  // namespace step::pipeline
