#pragma once

#include "node_graph.hpp"

#include "pipeline_thread.hpp"

#include <mutex>

namespace step::video::pipeline {

constexpr std::string_view PIPELINE_FIELD = "pipeline";

}

namespace step::video::pipeline {

class FramePipeline : public ISerializable
{
public:
    FramePipeline(const ObjectPtrJSON& config);
    ~FramePipeline();

    void deserialize(const ObjectPtrJSON& config);

private:
    void process_thread(const std::string& node_id, FramePipelineDataType&& data);

    void init_with_input_node();

private:
    FramePipelineGraphNode::Ptr m_root;
    PipelineThread m_main_thread;
    std::vector<PipelineThread> m_branch_threads;
};

}  // namespace step::video::pipeline