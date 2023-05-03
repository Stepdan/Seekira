#include <pipeline/nodes/empty_node.hpp>
#include <pipeline/nodes/input_node.hpp>

#include <abstract_task/settings_factory.hpp>
#include <abstract_task/abstract_task_factory.hpp>

#include <base/interfaces/event_handler_list.hpp>
#include <base/utils/thread/thread_pool_execute_policy.hpp>
#include <base/utils/json/json_utils.hpp>

#include <video/pipeline/frame_pipeline.hpp>

#include <application/registrator.hpp>

#include <log/log.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>

using namespace step;
using namespace step::video;
using namespace step::video::utils;

using namespace std::literals;

struct TestDataProvider
{
    static std::filesystem::path test_data_dir()
    {
#ifndef ONE_BRANCH_FRAME_PIPELINE_TESTS_DATA_DIR
#error "ONE_BRANCH_FRAME_PIPELINE_TESTS_DATA_DIR must be defined and point to valid testdata folder"
#endif
        std::filesystem::path path(ONE_BRANCH_FRAME_PIPELINE_TESTS_DATA_DIR);
        assert(std::filesystem::is_directory(path));
        return path;
    }

    static std::filesystem::path invalid_data_dir() { return test_data_dir().append("invalid"); }

    static ObjectPtrJSON open_pipeline_config(const std::string& pipeline_path)
    {
        return step::json::utils::from_file(pipeline_path);
    }
};

class FrameSource : video::IFrameSource
{
public:
    void register_observer(IFrameSourceObserver* observer) override
    {
        m_frame_observers.register_event_handler(observer);
    }

    void unregister_observer(IFrameSourceObserver* observer) override
    {
        m_frame_observers.unregister_event_handler(observer);
    }

    void create_and_process_frame()
    {
        auto frame_ptr = std::make_shared<Frame>();
        m_frame_observers.perform_for_each_event_handler(
            std::bind(&IFrameSourceObserver::process_frame, std::placeholders::_1, frame_ptr));
    }

private:
    step::EventHandlerList<video::IFrameSourceObserver, step::ThreadPoolExecutePolicy<0>> m_frame_observers;
};

class PipelineTest : public ::testing::Test
{
public:
    void SetUp()
    {
        step::app::Registrator::instance().initialize();
        m_pipeline = nullptr;
    }

    std::unique_ptr<FramePipeline> m_pipeline{nullptr};
};

TEST_F(PipelineTest, one_branch_pipeline_constructible_destructible)
{
    const auto init_pipeline = [this](const ObjectPtrJSON& cfg) { m_pipeline = std::make_unique<FramePipeline>(cfg); };

    const auto filename = "one_branch_pipeline.json";
    auto entry_path = TestDataProvider::test_data_dir().append(filename);
    STEP_LOG(L_INFO, "Processing PipelineTest with config: {}", filename);

    m_pipeline.reset();
    auto pipeline_cfg = TestDataProvider::open_pipeline_config(entry_path.string());

    EXPECT_NO_THROW(init_pipeline(pipeline_cfg));
}

TEST_F(PipelineTest, one_branch_pipeline_single_run)
{
    const auto init_pipeline = [this](const ObjectPtrJSON& cfg) { m_pipeline = std::make_unique<FramePipeline>(cfg); };

    const auto filename = "one_branch_pipeline.json";
    auto entry_path = TestDataProvider::test_data_dir().append(filename);
    STEP_LOG(L_INFO, "Processing PipelineTest with config: {}", filename);

    m_pipeline.reset();
    auto pipeline_cfg = TestDataProvider::open_pipeline_config(entry_path.string());

    EXPECT_NO_THROW(init_pipeline(pipeline_cfg));

    FrameSource source;
    source.register_observer(m_pipeline.get());
    EXPECT_NO_THROW(source.create_and_process_frame());

    std::this_thread::sleep_for(50ms);
}