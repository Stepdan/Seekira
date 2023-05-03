#include <abstract_task/settings_factory.hpp>
#include <abstract_task/abstract_task_factory.hpp>

#include <base/utils/json/json_utils.hpp>

#include <video/pipeline/frame_pipeline.hpp>

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
#ifndef INVALID_FRAME_PIPELINE_TESTS_DATA_DIR
#error "INVALID_FRAME_PIPELINE_TESTS_DATA_DIR must be defined and point to valid testdata folder"
#endif
        std::filesystem::path path(INVALID_FRAME_PIPELINE_TESTS_DATA_DIR);
        assert(std::filesystem::is_directory(path));
        return path;
    }

    static ObjectPtrJSON open_pipeline_config(const std::string& pipeline_path)
    {
        return step::json::utils::from_file(pipeline_path);
    }
};

class PipelineTest : public ::testing::Test
{
public:
    void SetUp() { m_pipeline = nullptr; }

    std::unique_ptr<FramePipeline> m_pipeline{nullptr};
};

#define STRING(s) #s

#define INVALID_PIPELINE_TEST(pipeline_file_and_test_name)                                                             \
    TEST_F(PipelineTest, pipeline_file_and_test_name)                                                                  \
    {                                                                                                                  \
        const auto init_pipeline = [this](const ObjectPtrJSON& cfg) {                                                  \
            m_pipeline = std::make_unique<FramePipeline>(cfg);                                                         \
        };                                                                                                             \
                                                                                                                       \
        const auto filename = std::string(STRING(pipeline_file_and_test_name)) + ".json";                              \
        auto entry_path = TestDataProvider::test_data_dir().append(filename);                                          \
        STEP_LOG(L_INFO, "Processing PipelineTest with invalid config: {}", STRING(pipeline_file_and_test_name));      \
                                                                                                                       \
        m_pipeline.reset();                                                                                            \
        auto pipeline_cfg = TestDataProvider::open_pipeline_config(entry_path.string());                               \
                                                                                                                       \
        EXPECT_THROW(init_pipeline(pipeline_cfg), std::runtime_error);                                                 \
    }

INVALID_PIPELINE_TEST(duplicate_links_pipeline)
INVALID_PIPELINE_TEST(duplicate_node_id_pipeline)
INVALID_PIPELINE_TEST(invalid_link_empty_pipeline)
INVALID_PIPELINE_TEST(invalid_link_one_node_pipeline)
INVALID_PIPELINE_TEST(invalid_link_three_nodes_pipeline)
INVALID_PIPELINE_TEST(no_input_node_pipeline)
INVALID_PIPELINE_TEST(non_existing_child_node_pipeline)
INVALID_PIPELINE_TEST(non_existing_parent_node_pipeline)
// TODO Check for loops
//INVALID_PIPELINE_TEST(loop_pipeline)
//INVALID_PIPELINE_TEST(self_loop_link_pipeline)