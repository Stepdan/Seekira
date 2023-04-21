#pragma once

#include "node.hpp"

#include <base/utils/json/json_types.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace step::video::pipeline {

using PipelineActivateThreadCallback = std::function<void(const std::string& node_id, FramePipelineDataType&&)>;

class FramePipelineGraphNode : public ISerializable
{
public:
    using Ptr = std::shared_ptr<FramePipelineGraphNode>;

public:
    FramePipelineGraphNode(const std::string& node_id, std::unique_ptr<IFramePipelineNode>&& node,
                           PipelineActivateThreadCallback callback);
    FramePipelineGraphNode(const ObjectPtrJSON& config, PipelineActivateThreadCallback callback);

    void process(FramePipelineDataType&& data);

    const std::string& get_id() const noexcept { return m_node_id; }

    void add_sink(const Ptr& sink) { m_sinks.push_back(sink); }

private:
    void deserialize(const ObjectPtrJSON&) override;

private:
    std::vector<Ptr> m_sinks;
    std::unique_ptr<IFramePipelineNode> m_node;

    std::string m_node_id;
    PipelineActivateThreadCallback m_activate_thread_callback;
};

}  // namespace step::video::pipeline
