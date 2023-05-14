#include <core/base/json/json_utils.hpp>

#include <video/ffmpeg/wrapper/ff_wrapper.hpp>

#include <core/log/log.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>

using namespace step;
using namespace step::video;
using namespace step::video::utils;
using namespace step::video::ff;

using namespace std::literals;

struct TestDataProvider
{
    static std::filesystem::path test_data_dir()
    {
#ifndef FF_TESTS_DATA_DIR
#error "FF_TESTS_DATA_DIR must be defined and point to valid testdata folder"
#endif
        std::filesystem::path path(FF_TESTS_DATA_DIR);
        assert(std::filesystem::is_directory(path));
        return path;
    }

    static ObjectPtrJSON open_config(const std::string& config_path)
    {
        return step::json::utils::from_file(config_path);
    }
};

class FrameSourceObserver : public video::IFrameSourceObserver
{
public:
    void process_frame(FramePtr frame_ptr) override { STEP_LOG(L_INFO, "Frame processing: {}", frame_ptr); }
};

class FFTest : public ::testing::Test
{
public:
    void SetUp() { step::log::Logger::instance().set_log_level(L_TRACE); }

    std::unique_ptr<FFWrapper> m_ff_wrapper;
    std::unique_ptr<IFrameSourceObserver> m_frame_observer;
};

TEST_F(FFTest, ff_wrapper_constructible_destructible)
{
    const auto init_ff_wrapper = [this]() {
        m_ff_wrapper = std::make_unique<FFWrapper>();
        m_frame_observer = std::make_unique<FrameSourceObserver>();
        m_ff_wrapper->register_observer(m_frame_observer.get());
    };

    EXPECT_NO_THROW(init_ff_wrapper());
}

TEST_F(FFTest, ff_wrapper_open_file)
{
    const auto init_ff_wrapper = [this]() {
        m_ff_wrapper = std::make_unique<FFWrapper>();
        m_frame_observer = std::make_unique<FrameSourceObserver>();
        m_ff_wrapper->register_observer(m_frame_observer.get());
    };

    EXPECT_NO_THROW(init_ff_wrapper());

    EXPECT_NO_THROW(m_ff_wrapper->open_file("C:/Work/test_video/IMG_5903.MOV"));
}

TEST_F(FFTest, ff_wrapper_playing_file)
{
    const auto init_ff_wrapper = [this]() {
        m_ff_wrapper = std::make_unique<FFWrapper>();
        m_frame_observer = std::make_unique<FrameSourceObserver>();
        m_ff_wrapper->register_observer(m_frame_observer.get());
    };

    EXPECT_NO_THROW(init_ff_wrapper());

    EXPECT_NO_THROW(m_ff_wrapper->open_file("C:/Work/test_video/IMG_5903.MOV"));

    EXPECT_NO_THROW(m_ff_wrapper->run());
    while (m_ff_wrapper->is_running())
    {
        std::this_thread::sleep_for(1s);
    }
}