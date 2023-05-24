#include <core/log/log.hpp>

#include <core/base/utils/string_utils.hpp>
#include <core/base/json/json_utils.hpp>

#include <video/frame/utils/frame_utils.hpp>

#include <video/ffmpeg/player/player.hpp>

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
    void process_frame(FramePtr frame_ptr) override
    {
        STEP_LOG(L_INFO, "Frame processing: {}", frame_ptr);
        if (frame_ptr && frame_ptr->ts.count() > 4000000 && frame_ptr->ts.count() < 5000000)
           step::video::utils::save_to_file(*frame_ptr, "C:/Work/test_video/image.png");
    }
};

class FFTest : public ::testing::Test, public IPlayerEventObserver
{
public:
    void SetUp() { step::log::Logger::instance().set_log_level(L_TRACE); }

    void on_media_status_changed(MediaStatus status)
    {
        STEP_LOG(L_INFO, "Media status changed to {}", step::utils::to_string(status));
    }

    void on_playing_state_changed(PlayingState state)
    {
        STEP_LOG(L_INFO, "Playing state changed to {}", step::utils::to_string(state));
        if (state != PlayingState::Playing)
            m_is_playing_finished = true;
    }

    std::unique_ptr<PlayerFF> m_player;
    std::unique_ptr<IFrameSourceObserver> m_frame_observer;

    bool m_is_playing_finished{false};
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

    const auto open_file = [this]() {
        try
        {
            m_player->open_file("C:/Work/test_video/IMG_5903.MOV");
        }
        catch (std::exception& e)
        {
            STEP_LOG(L_ERROR, "Handle exception during opening file: {}", e.what());
            std::rethrow_exception(std::current_exception());
        }
        catch (...)
        {
            STEP_LOG(L_ERROR, "Handle unknown exception during opening file");
            std::rethrow_exception(std::current_exception());
        }
    };

    EXPECT_NO_THROW(open_file());
}

TEST_F(FFTest, ff_player_play_video)
{
    const auto init_ff_player = [this]() {
        m_player = std::make_unique<PlayerFF>();
        m_frame_observer = std::make_unique<FrameSourceObserver>();
        m_player->register_observer(m_frame_observer.get());
    };

    EXPECT_NO_THROW(init_ff_player());

    const auto open_file = [this]() {
        try
        {
            m_player->open_file("C:/Work/test_video/IMG_5903.MOV");
        }
        catch (std::exception& e)
        {
            STEP_LOG(L_ERROR, "Handle exception during opening file: {}", e.what());
            std::rethrow_exception(std::current_exception());
        }
        catch (...)
        {
            STEP_LOG(L_ERROR, "Handle unknown exception during opening file");
            std::rethrow_exception(std::current_exception());
        }
    };

    EXPECT_NO_THROW(open_file());

    m_player->register_observer(m_frame_observer.get());
    m_player->register_observer(this);

    EXPECT_NO_THROW(m_player->play());
    while (!m_is_playing_finished)
        std::this_thread::sleep_for(50ms);
}