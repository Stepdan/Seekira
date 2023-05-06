#include <video/frame/utils/frame_utils.hpp>

#include <gui/utils/qimage_utils.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <set>

#include <log/log.hpp>

using namespace step;
using namespace step::video;
using namespace step::video::utils;

struct TestDataProvider
{
    static std::filesystem::path test_data_dir()
    {
#ifndef GUI_IMAGE_UTILS_TESTS_DATA_DIR
#error "GUI_IMAGE_UTILS_TESTS_DATA_DIR must be defined and point to valid testdata folder"
#endif
        std::filesystem::path path(GUI_IMAGE_UTILS_TESTS_DATA_DIR);
        assert(std::filesystem::is_directory(path));
        return path;
    }

    // hardcoded image.png data for tests usage
    static std::string frame_path() { return test_data_dir().append("lena.jpg").string(); }
};

class FrameTest : public ::testing::Test
{
protected:
    void SetUp() { frame = video::utils::open_file(TestDataProvider::frame_path()); }

    Frame frame;
};

TEST_F(FrameTest, frame_conversions)
{
    step::video::PixFmt allowed_formats[] = {
        step::video::PixFmt::BGR,
        step::video::PixFmt::RGB,
        step::video::PixFmt::RGBA,
        step::video::PixFmt::GRAY,
    };

    for (const auto& fmt : allowed_formats)
    {
        auto converted = frame;
        video::utils::convert_colorspace(converted, fmt);
        auto qimage = gui::utils::frame_to_qimage_deep(converted);
        auto frame_from_qimage = gui::utils::qimage_to_frame(qimage);
        ASSERT_EQ(converted, frame_from_qimage);
    }

    step::video::PixFmt not_allowed_formats[] = {
        step::video::PixFmt::BGRA,
    };

    for (const auto& fmt : not_allowed_formats)
    {
        auto converted = frame;
        video::utils::convert_colorspace(converted, fmt);
        EXPECT_ANY_THROW(gui::utils::frame_to_qimage_deep(converted));
    }
}