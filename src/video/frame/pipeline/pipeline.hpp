#pragma once

#include "pipeline_thread_pool.hpp"

namespace step::video::pipeline {

class FramePipeline : public ISerializable
{
public:
    FramePipeline(const ObjectPtrJSON& config);
    ~FramePipeline();

    void deserialize(const ObjectPtrJSON& config);

private:
    void reset();
    void process_thread(const std::string& node_id, FramePipelineDataType&& data);

    bool is_thread_pool_exist() const noexcept { return !!m_thread_pool; }

private:
    std::shared_ptr<FramePipelineSettings> m_settings{nullptr};
    FramePipelineGraphNode::Ptr m_root{nullptr};

    std::unique_ptr<PipelineThreadPool> m_thread_pool{nullptr};
};

}  // namespace step::video::pipeline