#pragma once

#include "node_graph.hpp"
#include "pipeline_settings.hpp"

#include <atomic>
#include <mutex>
#include <thread>

namespace step::video::pipeline {

class PipelineThread
{
public:
    PipelineThread(const std::shared_ptr<FramePipelineGraphNode>& root_node);

    PipelineThread(PipelineThread&&) = default;
    PipelineThread& operator=(PipelineThread&&) = default;

    PipelineThread(const PipelineThread&) = delete;
    PipelineThread& operator=(const PipelineThread&) = delete;

    const std::string& get_root_node_id() const noexcept;

    bool is_running() const { return m_is_running; }

    void run(FramePipelineDataType&& data);

    bool joinable();
    void join();

private:
    void run_impl(FramePipelineDataType&& data);

private:
    std::shared_ptr<FramePipelineGraphNode> m_root_node;

    std::mutex m_guard;
    std::thread m_worker;
    std::atomic_bool m_is_running{false};
};

}  // namespace step::video::pipeline