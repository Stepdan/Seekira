#include <core/base/json/json_utils.hpp>

#include <video/ffmpeg/player/player.hpp>

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

    std::unique_ptr<PlayerFF> m_player;
    std::unique_ptr<IFrameSourceObserver> m_frame_observer;
};

TEST_F(FFTest, ff_player_constructible_destructible)
{
    const auto init_ff_player = [this]() {
        m_player = std::make_unique<PlayerFF>();
        m_frame_observer = std::make_unique<FrameSourceObserver>();
        m_player->register_observer(m_frame_observer.get());
    };

    EXPECT_NO_THROW(init_ff_player());
}

TEST_F(FFTest, ff_player_open_file)
{
    const auto init_ff_player = [this]() {
        m_player = std::make_unique<PlayerFF>();
        m_frame_observer = std::make_unique<FrameSourceObserver>();
        m_player->register_observer(m_frame_observer.get());
    };

    EXPECT_NO_THROW(init_ff_player());

    EXPECT_NO_THROW(m_player->open_file("C:/Work/test_video/IMG_5903.MOV"));
}

TEST_F(FFTest, ff_player_seek)
{
    const auto init_ff_player = [this]() {
        m_player = std::make_unique<PlayerFF>();
        m_frame_observer = std::make_unique<FrameSourceObserver>();
        m_player->register_observer(m_frame_observer.get());
    };

    EXPECT_NO_THROW(init_ff_player());

    EXPECT_NO_THROW(m_player->open_file("C:/Work/test_video/IMG_5903.MOV"));
}