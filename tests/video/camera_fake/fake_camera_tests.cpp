#include <video/camera/camera_fake/fake_camera.hpp>

#include <video/frame/utils/frame_utils.hpp>
#include <video/frame/utils/frame_utils_opencv.hpp>

#include <core/threading/thread_guard.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <vector>

using namespace std::literals;

struct TestDataProvider
{
    TestDataProvider()
    {
        for (const auto& entry : std::filesystem::directory_iterator(test_data_dir()))
        {
            auto entry_path = entry.path();
            auto frame = step::video::utils::open_file(entry_path.string(), step::video::PixFmt::BGR);
            if (frame.is_valid())
            {
                m_reference_frames.push_back(std::move(frame));
            }
        }
    }

    static std::filesystem::path test_data_dir()
    {
#ifndef CAMERA_FAKE_TESTS_DATA_DIR
#error "CAMERA_FAKE_TESTS_DATA_DIR must be defined and point to valid test data dir"
#endif
        std::filesystem::path path(CAMERA_FAKE_TESTS_DATA_DIR);
        assert(std::filesystem::is_directory(path));
        return path;
    }

    static std::filesystem::path test_data_random_test_file() { return test_data_dir().append("lena.jpg"); }

protected:
    std::vector<step::video::Frame> m_reference_frames;
};

struct FrameHandler : public step::video::IFrameSourceObserver
{
    step::threading::ThreadGuard<step::video::FramesPtrs> captured_frames;

    void process_frame(step::video::FramePtr frame_ptr) override { captured_frames->push_back(frame_ptr); }
};

struct FrameHandlerAsync : public step::video::IFrameSourceObserver
{
    step::threading::ThreadGuard<step::video::FramesPtrs> captured_frames;
    std::mutex captured_frames_mutex;

    void process_frame(step::video::FramePtr frame_ptr) override
    {
        std::lock_guard g(captured_frames_mutex);
        captured_frames->push_back(frame_ptr);
    }

    const auto get_captured_frames_size()
    {
        std::lock_guard g(captured_frames_mutex);
        return captured_frames->size();
    };
};

struct FrameHandlerEmpty : public step::video::IFrameSourceObserver
{
    void process_frame(step::video::FramePtr) override {}
};

struct FrameHandlerOneFrame : public step::video::IFrameSourceObserver
{
    step::threading::ThreadGuard<step::video::FramePtr> frame;

    void process_frame(step::video::FramePtr frame_ptr) override { *frame.lock() = frame_ptr; }
};

struct FakeCameraSingleshotTest : TestDataProvider, ::testing::Test
{
};

struct FakeCameraStreamingTest : TestDataProvider, ::testing::Test
{
};

static bool exhausted(const std::unique_ptr<step::video::ICameraSingleshot>& cam)
{
    auto downcast = dynamic_cast<step::video::FakeCameraSingleshot*>(cam.get());
    assert(downcast != nullptr);
    return downcast->exhausted();
}

static bool exhausted(const std::unique_ptr<step::video::ICameraStreaming>& cam)
{
    auto downcast = dynamic_cast<step::video::FakeCameraStreamer*>(cam.get());
    assert(downcast != nullptr);
    return downcast->exhausted();
}

TEST_F(FakeCameraSingleshotTest, constructible_destructible)
{
    step::video::FakeCameraSettings settings(TestDataProvider::test_data_dir());
    auto cam = create_camera_singleshot(settings);
    ASSERT_NE(cam, nullptr);
}

TEST_F(FakeCameraSingleshotTest, get_frame_single_file)
{
    step::video::FakeCameraSettings settings(TestDataProvider::test_data_random_test_file());
    auto cam = create_camera_singleshot(settings);
    ASSERT_TRUE(!exhausted(cam));

    auto frame = cam->get_frame();
    ASSERT_TRUE(frame.is_valid());
    ASSERT_EQ(frame, m_reference_frames[0]);

    ASSERT_TRUE(exhausted(cam));

    EXPECT_ANY_THROW(cam->get_frame()) << "Exhausted camera didn't fail on get_frame()";
}

TEST_F(FakeCameraSingleshotTest, get_frame)
{
    step::video::FakeCameraSettings settings(TestDataProvider::test_data_dir());
    auto cam = create_camera_singleshot(settings);
    ASSERT_TRUE(!exhausted(cam));

    auto frame1 = cam->get_frame();
    ASSERT_TRUE(frame1.is_valid());
    ASSERT_EQ(frame1, m_reference_frames[0]);

    auto frame2 = cam->get_frame();
    ASSERT_TRUE(frame2.is_valid());
    ASSERT_EQ(frame2, m_reference_frames[1]);

    ASSERT_TRUE(exhausted(cam));

    EXPECT_ANY_THROW(cam->get_frame()) << "Exhausted camera didn't fail on get_frame()";
}

TEST_F(FakeCameraSingleshotTest, get_frame_cyclic)
{
    step::video::FakeCameraSettings settings(TestDataProvider::test_data_dir(), true);
    auto cam = create_camera_singleshot(settings);
    ASSERT_TRUE(!exhausted(cam));

    for (int repeats = 0; repeats < 10; ++repeats)
    {
        auto frame1 = cam->get_frame();
        ASSERT_TRUE(frame1.is_valid());
        ASSERT_EQ(frame1, m_reference_frames[0]);

        auto frame2 = cam->get_frame();
        ASSERT_TRUE(frame2.is_valid());
        ASSERT_EQ(frame2, m_reference_frames[1]);

        ASSERT_TRUE(!exhausted(cam));
    }

    ASSERT_TRUE(!exhausted(cam));
}

TEST_F(FakeCameraSingleshotTest, varying_pixel_format)
{
    step::video::PixFmt formats[] = {
        step::video::PixFmt::BGR,  step::video::PixFmt::RGB,  step::video::PixFmt::RGBA,
        step::video::PixFmt::BGRA, step::video::PixFmt::GRAY,
    };

    for (auto format : formats)
    {
        step::video::FakeCameraSettings settings(TestDataProvider::test_data_random_test_file());
        settings.pix_fmt = format;
        auto cam = create_camera_singleshot(settings);
        ASSERT_TRUE(!exhausted(cam));
        auto frame = cam->get_frame();
        ASSERT_TRUE(exhausted(cam));

        ASSERT_TRUE(frame.is_valid());
        ASSERT_EQ(frame.size, m_reference_frames[0].size);

        using namespace step::video::utils;
        switch (format)
        {
            case step::video::PixFmt::RGB:
                [[fallthrough]];
            case step::video::PixFmt::BGR: {
                ASSERT_EQ(get_cv_data_type(frame.pix_fmt), CV_8UC3);
                break;
            }

            case step::video::PixFmt::GRAY: {
                ASSERT_EQ(get_cv_data_type(frame.pix_fmt), CV_8UC1);
                break;
            }

            case step::video::PixFmt::RGBA:
                [[fallthrough]];
            case step::video::PixFmt::BGRA: {
                ASSERT_EQ(get_cv_data_type(frame.pix_fmt), CV_8UC4);
                break;
            }
            default: {
                GTEST_FAIL() << "Unexpected format!";
                break;
            }
        }
    }
}

TEST_F(FakeCameraStreamingTest, constructible_destructible)
{
    step::video::FakeCameraSettings settings(TestDataProvider::test_data_dir());
    auto cam = create_camera_streaming(settings);
    ASSERT_NE(cam, nullptr);
}

TEST_F(FakeCameraStreamingTest, stream_single_frame)
{
    step::video::FakeCameraSettings settings(TestDataProvider::test_data_random_test_file());
    auto cam = create_camera_streaming(settings);
    ASSERT_TRUE(!exhausted(cam));

    FrameHandler frame_handler;
    cam->register_observer(&frame_handler);

    std::this_thread::sleep_for(60ms);
    ASSERT_TRUE(frame_handler.captured_frames->empty());

    cam->start();  // kickstarts the streaming thread
    while (!exhausted(cam))
    {
        std::this_thread::sleep_for(20ms);
    }

    ASSERT_TRUE(exhausted(cam));

    {
        auto frames = frame_handler.captured_frames.lock();
        ASSERT_EQ(frames->size(), 1);

        ASSERT_TRUE(!!frames.get()[0]);
        ASSERT_TRUE(frames.get()[0]->is_valid());
        ASSERT_EQ(*frames.get()[0], m_reference_frames[0]);
    }

    EXPECT_ANY_THROW(cam->start()) << "Exhausted camera didn't fail on start()";
}

TEST_F(FakeCameraStreamingTest, stream_frames)
{
    step::video::FakeCameraSettings settings(TestDataProvider::test_data_dir());
    auto cam = create_camera_streaming(settings);
    ASSERT_TRUE(!exhausted(cam));

    FrameHandler frame_handler;
    cam->register_observer(&frame_handler);

    std::this_thread::sleep_for(60ms);
    ASSERT_TRUE(frame_handler.captured_frames->empty());

    cam->start();  // kickstarts the streaming thread
    while (!exhausted(cam))
    {
        std::this_thread::sleep_for(20ms);
    }

    ASSERT_TRUE(exhausted(cam));
    {
        auto frames = frame_handler.captured_frames.lock();
        ASSERT_EQ(frames->size(), 2);

        ASSERT_TRUE(!!frames.get()[0]);
        ASSERT_TRUE(frames.get()[0]->is_valid());
        ASSERT_EQ(*frames.get()[0], m_reference_frames[0]);

        ASSERT_TRUE(!!frames.get()[1]);
        ASSERT_TRUE(frames.get()[1]->is_valid());
        ASSERT_EQ(*frames.get()[1], m_reference_frames[1]);
    }

    EXPECT_ANY_THROW(cam->start()) << "Exhausted camera didn't fail on start()";
}

TEST_F(FakeCameraStreamingTest, stream_frames_cyclic)
{
    step::video::FakeCameraSettings settings(TestDataProvider::test_data_dir(), true);
    settings.frame_rate = 120;
    auto cam = create_camera_streaming(settings);
    ASSERT_TRUE(!exhausted(cam));

    FrameHandlerAsync frame_handler;
    cam->register_observer(&frame_handler);

    std::this_thread::sleep_for(60ms);
    ASSERT_TRUE(frame_handler.captured_frames.lock()->empty());

    cam->start();  // kickstarts the streaming thread
    for (int i = 0; i < 1000; ++i)
    {
        if (frame_handler.get_captured_frames_size() >= 10)
        {
            cam->stop();  // stops the streaming thread
            break;
        }
        std::this_thread::sleep_for(10ms);
    }

    const size_t final_size = frame_handler.get_captured_frames_size();
    std::this_thread::sleep_for(100ms);
    ASSERT_EQ(final_size, frame_handler.get_captured_frames_size());  // ensure the streaming is stopped

    ASSERT_TRUE(!exhausted(cam));

    for (size_t i = 0; i < final_size; ++i)
    {
        const auto& reference_frame = m_reference_frames[i % 2];
        ASSERT_TRUE(!!frame_handler.captured_frames.lock().get()[i]);
        ASSERT_TRUE(frame_handler.captured_frames.lock().get()[i]->is_valid());
        ASSERT_EQ(*frame_handler.captured_frames.lock().get()[i], reference_frame);
    }

    try
    {
        cam->start();
        std::this_thread::sleep_for(50ms);
        cam->stop();
        ASSERT_TRUE(!exhausted(cam));
    }
    catch (const std::runtime_error&)
    {
        GTEST_FAIL() << "Cyclic camera failed to restart";
    }
}

TEST_F(FakeCameraStreamingTest, stream_frames_max_fps)
{
    step::video::FakeCameraSettings settings(TestDataProvider::test_data_dir());
    // even if fps is enormous, there shouldn't be any infinite waits
    settings.frame_rate = std::numeric_limits<int>::max();
    auto cam = create_camera_streaming(settings);
    ASSERT_TRUE(!exhausted(cam));

    FrameHandlerEmpty frame_handler;
    cam->register_observer(&frame_handler);

    cam->start();  // kickstarts the streaming thread
    while (!exhausted(cam))
    {
        std::this_thread::sleep_for(2ms);
    }
}

TEST_F(FakeCameraStreamingTest, varying_pixel_format)
{
    step::video::PixFmt formats[] = {
        step::video::PixFmt::BGR,  step::video::PixFmt::RGB,  step::video::PixFmt::RGBA,
        step::video::PixFmt::BGRA, step::video::PixFmt::GRAY,
    };

    for (auto format : formats)
    {
        step::video::FakeCameraSettings settings(TestDataProvider::test_data_random_test_file());
        settings.pix_fmt = format;
        auto cam = create_camera_streaming(settings);
        ASSERT_TRUE(!exhausted(cam));

        FrameHandlerOneFrame frame_handler;
        cam->register_observer(&frame_handler);

        cam->start();
        while (!exhausted(cam))
        {
            std::this_thread::sleep_for(30ms);
        }
        ASSERT_TRUE(exhausted(cam));

        {
            auto frame = frame_handler.frame.lock();
            ASSERT_TRUE(!!frame.get());
            ASSERT_TRUE(frame->is_valid());
            ASSERT_EQ(frame->size, m_reference_frames[0].size);

            using namespace step::video::utils;
            switch (format)
            {
                case step::video::PixFmt::RGB:
                    [[fallthrough]];
                case step::video::PixFmt::BGR: {
                    ASSERT_EQ(get_cv_data_type(frame->pix_fmt), CV_8UC3);
                    break;
                }

                case step::video::PixFmt::GRAY: {
                    ASSERT_EQ(get_cv_data_type(frame->pix_fmt), CV_8UC1);
                    break;
                }

                case step::video::PixFmt::RGBA:
                    [[fallthrough]];
                case step::video::PixFmt::BGRA: {
                    ASSERT_EQ(get_cv_data_type(frame->pix_fmt), CV_8UC4);
                    break;
                }
                default: {
                    GTEST_FAIL() << "Unexpected format!";
                    break;
                }
            }
        }
    }
}
