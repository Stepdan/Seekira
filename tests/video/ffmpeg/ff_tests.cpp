#include <core/log/log.hpp>

#include <core/base/utils/string_utils.hpp>
#include <core/base/json/json_utils.hpp>

#include <video/frame/utils/frame_utils.hpp>

#include <video/ffmpeg/reader/reader.hpp>

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
        //if (frame_ptr && frame_ptr->ts.count() > 4000000 && frame_ptr->ts.count() < 5000000)
        //    step::video::utils::save_to_file(*frame_ptr, "C:/Work/test_video/image.png");
    }
};

class FFTest : public ::testing::Test, public IReaderEventObserver
{
public:
    void SetUp() { step::log::Logger::instance().set_log_level(L_INFO); }

    void on_reader_state_changed(ReaderState state)
    {
        STEP_LOG(L_INFO, "Reader state has been changed to {}", step::utils::to_string(state));
        if (state == ReaderState::EndOfFile)
            m_is_reading_finished = true;

        if (state == ReaderState::Error || state == ReaderState::InvalidSeek || state == ReaderState::Undefined)
        {
            STEP_LOG(L_ERROR, "Invalid state, break!");
            m_is_reading_finished = true;
        }
    }

    void init_reader()
    {
        m_reader = std::make_unique<ReaderFF>();
        m_frame_observer = std::make_unique<FrameSourceObserver>();
        m_reader->register_observer(m_frame_observer.get());
    }

    void open_file(const std::string& filepath)
    {
        try
        {
            m_reader->open_file(filepath);
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
    }

    std::unique_ptr<ReaderFF> m_reader;
    std::unique_ptr<IFrameSourceObserver> m_frame_observer;

    bool m_is_reading_finished{false};
};

TEST_F(FFTest, ff_reader_constructible_destructible) { EXPECT_NO_THROW(init_reader()); }

TEST_F(FFTest, ff_reader_open_file)
{
    EXPECT_NO_THROW(init_reader());

    const std::string filepath = "C:/Work/test_video/IMG_5903.MOV";
    EXPECT_NO_THROW(open_file(filepath));
}

TEST_F(FFTest, ff_reader_continuously_reading)
{
    EXPECT_NO_THROW(init_reader());

    const std::string filepath = "C:/Work/test_video/IMG_5903.MOV";
    EXPECT_NO_THROW(open_file(filepath));

    m_reader->register_observer(m_frame_observer.get());
    m_reader->register_observer(this);

    EXPECT_NO_THROW(m_reader->start());
    EXPECT_NO_THROW(m_reader->play());
    while (!m_is_reading_finished)
        std::this_thread::sleep_for(50ms);
}

TEST_F(FFTest, ff_reader_request_reading)
{
    EXPECT_NO_THROW(init_reader());

    const std::string filepath = "C:/Work/test_video/IMG_5903.MOV";
    EXPECT_NO_THROW(open_file(filepath));

    m_reader->register_observer(m_frame_observer.get());
    m_reader->register_observer(this);

    EXPECT_NO_THROW(m_reader->start());

    for (size_t i = 0; i < 10; ++i)
    {
        if (m_is_reading_finished)
            break;

        EXPECT_NO_THROW(m_reader->request_read());
    }
}

TEST_F(FFTest, ff_reader_single_seek)
{
    EXPECT_NO_THROW(init_reader());

    const std::string filepath = "C:/Work/test_video/IMG_5903.MOV";
    EXPECT_NO_THROW(open_file(filepath));

    m_reader->register_observer(m_frame_observer.get());
    m_reader->register_observer(this);

    EXPECT_NO_THROW(m_reader->start());
    EXPECT_NO_THROW(m_reader->set_position(m_reader->get_duration() / 2));
    EXPECT_NO_THROW(m_reader->request_read());
}

TEST_F(FFTest, ff_reader_seek_to_zero)
{
    EXPECT_NO_THROW(init_reader());

    const std::string filepath = "C:/Work/test_video/IMG_5903.MOV";
    EXPECT_NO_THROW(open_file(filepath));

    m_reader->register_observer(m_frame_observer.get());
    m_reader->register_observer(this);

    EXPECT_NO_THROW(m_reader->start());
    EXPECT_NO_THROW(m_reader->set_position(m_reader->get_duration() / 2));
    EXPECT_NO_THROW(m_reader->request_read());
    EXPECT_NO_THROW(m_reader->set_position(0));
    EXPECT_NO_THROW(m_reader->request_read());
}

TEST_F(FFTest, ff_reader_seek_to_end)
{
    EXPECT_NO_THROW(init_reader());

    const std::string filepath = "C:/Work/test_video/IMG_5903.MOV";
    EXPECT_NO_THROW(open_file(filepath));

    m_reader->register_observer(m_frame_observer.get());
    m_reader->register_observer(this);

    EXPECT_NO_THROW(m_reader->start());
    EXPECT_NO_THROW(m_reader->set_position(m_reader->get_duration()));
    EXPECT_NO_THROW(m_reader->request_read());

    ASSERT_EQ(ReaderState::EndOfFile, m_reader->get_state());
}

TEST_F(FFTest, ff_reader_seek_over_the_end)
{
    EXPECT_NO_THROW(init_reader());

    const std::string filepath = "C:/Work/test_video/IMG_5903.MOV";
    EXPECT_NO_THROW(open_file(filepath));

    m_reader->register_observer(m_frame_observer.get());
    m_reader->register_observer(this);

    EXPECT_NO_THROW(m_reader->start());
    EXPECT_NO_THROW(m_reader->set_position(m_reader->get_duration() + m_reader->get_duration() / 10));
    EXPECT_NO_THROW(m_reader->request_read());

    ASSERT_EQ(ReaderState::EndOfFile, m_reader->get_state());
}