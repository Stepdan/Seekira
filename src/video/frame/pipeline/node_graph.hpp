#pragma once

#include "node.hpp"

#include <base/utils/json/json_types.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace step::video::pipeline {

using PipelineActivateSinkCallback = std::function<void(const std::string& node_id, FramePipelineDataType&&)>;
using PipelineNodeFinishedCallback = std::function<void(const std::string& node_id)>;

class FramePipelineGraphNode : public ISerializable
{
public:
    using Ptr = std::shared_ptr<FramePipelineGraphNode>;

public:
    FramePipelineGraphNode(const std::string& node_id, std::unique_ptr<IFramePipelineNode>&& node);
    FramePipelineGraphNode(const ObjectPtrJSON& config);

    void process(FramePipelineDataType&& data);

    const std::string& get_id() const noexcept { return m_node_id; }
    std::optional<std::string> get_parent_id_opt() const noexcept;
    std::vector<std::string> get_sinks_id() const;

private:
    friend class FramePipeline;
    void set_parent(const Ptr& parent) { m_parent = parent; }
    void add_sink(const Ptr& sink) { m_sinks.push_back(sink); }

    void set_activate_sink_callback(PipelineActivateSinkCallback callback);
    void set_node_finished_callback(PipelineNodeFinishedCallback callback);

private:
    void deserialize(const ObjectPtrJSON&) override;

private:
    Ptr m_parent;
    std::vector<Ptr> m_sinks;
    std::unique_ptr<IFramePipelineNode> m_node;

    std::string m_node_id;
    PipelineActivateSinkCallback m_activate_sink_callback;
    PipelineNodeFinishedCallback m_finish_callback;
};

}  // namespace step::video::pipeline
