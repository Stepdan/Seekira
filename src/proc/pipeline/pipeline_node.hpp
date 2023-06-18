#pragma once

#include "pipeline_task.hpp"

#include <core/log/log.hpp>

#include <core/base/json/json_utils.hpp>
#include <core/task/settings_factory.hpp>
#include <core/task/task_factory.hpp>
#include <core/graph/node.hpp>

namespace step::proc {

using PipelineIdType = graph::BaseGraphNode::IdType;

template <typename TData>
class PipelineNode : public graph::BaseGraphNode, public ISerializable
{
public:
    using Ptr = std::shared_ptr<PipelineNode>;

public:
    PipelineNode(const PipelineIdType& node_id, std::unique_ptr<IPipelineNodeTask<TData>>&& task)
        : m_task(std::move(task))
    {
        m_id = node_id;
    }

    PipelineNode(const ObjectPtrJSON& config) { deserialize(config); }

    void process(const PipelineDataPtr<TData>& data)
    {
        STEP_LOG(L_TRACE, "Processing pipeline node {}", m_id);
        m_task->process(data);
    }

private:
    void deserialize(const ObjectPtrJSON& container) override
    {
        m_id = json::get<PipelineIdType>(container, CFG_FLD::NODE);

        const auto task_settings_json = json::get_object(container, CFG_FLD::SETTINGS);
        auto base_task_settings = task::TaskSettingsFactory::instance().create(task_settings_json);

        m_task = IPipelineNodeTask<TData>::from_abstract(CREATE_TASK_UNIQUE(base_task_settings));
    }

private:
    mutable std::unique_ptr<IPipelineNodeTask<TData>> m_task;
};

template <typename TData>
using PipelineNodePtr = std::shared_ptr<PipelineNode<TData>>;

}  // namespace step::proc
