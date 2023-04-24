#pragma once

#include "pipeline_settings.hpp"
#include "pipeline_thread.hpp"

#include <unordered_map>

namespace step::video::pipeline {

class PipelineThreadPool
{
public:
    PipelineThreadPool();
    ~PipelineThreadPool();

    void set_pipeline_settings(const std::shared_ptr<FramePipelineSettings>& settings);
    void set_main_thread_id(const std::string& id) { m_main_thread_id = id; }

    void add_thread(std::unique_ptr<PipelineThread>&& thread);

    void run(const std::string& thread_id, FramePipelineDataType&& data);
    void stop(const std::string& thread_id);
    void reset();

private:
    void run_sync(const std::string& thread_id, FramePipelineDataType&& data);
    void run_parallel_wait(const std::string& thread_id, FramePipelineDataType&& data);
    void run_parallel_no_wait(const std::string& thread_id, FramePipelineDataType&& data);

private:
    std::unordered_map<std::string, std::unique_ptr<PipelineThread>> m_threads;
    std::weak_ptr<FramePipelineSettings> m_pipeline_settings;

    std::mutex m_guard;

    std::string m_main_thread_id;
};

}  // namespace step::video::pipeline