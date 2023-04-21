#pragma once

#include "node_graph.hpp"

#include <atomic>
#include <thread>

namespace step::video::pipeline {

class PipelineThread
{
public:
    PipelineThread() = default;
    PipelineThread(const std::shared_ptr<FramePipelineGraphNode>& root_node);

    const std::string& get_root_node_id() const noexcept;

    void run(FramePipelineDataType&& data);

private:
    std::shared_ptr<FramePipelineGraphNode> m_root_node;
    std::thread m_worker;
    std::atomic_bool m_is_blocked{false};
};

}  // namespace step::video::pipeline