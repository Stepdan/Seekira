#include "pipeline_thread.hpp"

namespace step::video::pipeline {

PipelineThread::PipelineThread(const std::shared_ptr<FramePipelineGraphNode>& root_node) : m_root_node(root_node) {}

const std::string& PipelineThread::get_root_node_id() const noexcept { return m_root_node->get_id(); }

void PipelineThread::run(FramePipelineDataType&& data)
{
    m_worker = std::thread(&PipelineThread::run_impl, this, std::move(data));
    m_is_running = true;
}

void PipelineThread::run_impl(FramePipelineDataType&& data) { m_root_node->process(std::move(data)); }

bool PipelineThread::joinable() { return m_worker.joinable(); }

void PipelineThread::join()
{
    m_worker.join();
    m_is_running = false;
}

}  // namespace step::video::pipeline