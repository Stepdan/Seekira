#include "pipeline_thread.hpp"

namespace step::video::pipeline {

PipelineThread::PipelineThread(const std::shared_ptr<FramePipelineGraphNode>& root_node) {}

const std::string& PipelineThread::get_root_node_id() const noexcept { return m_root_node->get_id(); }

void PipelineThread::run(FramePipelineDataType&& data) {}

}  // namespace step::video::pipeline