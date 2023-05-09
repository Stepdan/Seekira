#include <video/frame/utils/frame_utils.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <set>

#include <core/log/log.hpp>

using namespace step;
using namespace step::video;
using namespace step::video::utils;

struct TestDataProvider
{
    static std::filesystem::path test_data_dir()
    {
#ifndef FRAME_TESTS_DATA_DIR
#error "FRAME_TESTS_DATA_DIR must be defined and point to valid testdata folder"
#endif
        std::filesystem::path path(FRAME_TESTS_DATA_DIR);
        assert(std::filesystem::is_directory(path));
        return path;
    }

    // hardcoded image.png data for tests usage
    static std::string frame_path() { return test_data_dir().append("lena.jpg").string(); }
    static PixFmt pixel_format() { return PixFmt::BGR; }
    static FrameSize frame_size() { return FrameSize(384, 384); }
    static size_t frame_bytesize()
    {
        return frame_size().width * frame_size().height * get_channels_count(pixel_format());
    }

    static std::string saved_frame_path() { return test_data_dir().append("saved_image.png").string(); }
};

class FrameTest : public ::testing::Test
{
protected:
    void SetUp() { frame = video::utils::open_file(TestDataProvider::frame_path()); }

    Frame frame;
};

TEST(PixFmtTest, pixel_format)
{
    std::set<PixFmt> tested_fmts;
    tested_fmts.insert(PixFmt::EndOf);

    const auto unsupported_channels_count = get_channels_count(PixFmt::Undefined);
    const auto unsupported_bpp = get_bpp(PixFmt::Undefined);

    // Undefined
    ASSERT_EQ(get_bpp(PixFmt::Undefined), 0);
    ASSERT_EQ(get_channels_count(PixFmt::Undefined), 0);
    ASSERT_FALSE(has_alpha(PixFmt::Undefined));
    tested_fmts.insert(PixFmt::Undefined);

    // GRAY
    ASSERT_EQ(get_bpp(PixFmt::GRAY), 8);
    ASSERT_EQ(get_channels_count(PixFmt::GRAY), 1);
    ASSERT_FALSE(has_alpha(PixFmt::GRAY));
    tested_fmts.insert(PixFmt::GRAY);

    // BGR
    ASSERT_EQ(get_bpp(PixFmt::BGR), 24);
    ASSERT_EQ(get_channels_count(PixFmt::BGR), 3);
    ASSERT_FALSE(has_alpha(PixFmt::BGR));
    tested_fmts.insert(PixFmt::BGR);

    // BGRA
    ASSERT_EQ(get_bpp(PixFmt::BGRA), 32);
    ASSERT_EQ(get_channels_count(PixFmt::BGRA), 4);
    ASSERT_TRUE(has_alpha(PixFmt::BGRA));
    tested_fmts.insert(PixFmt::BGRA);

    // RGB
    ASSERT_EQ(get_bpp(PixFmt::RGB), 24);
    ASSERT_EQ(get_channels_count(PixFmt::RGB), 3);
    ASSERT_FALSE(has_alpha(PixFmt::RGB));
    tested_fmts.insert(PixFmt::RGB);

    // RGBA
    ASSERT_EQ(get_bpp(PixFmt::RGBA), 32);
    ASSERT_EQ(get_channels_count(PixFmt::RGBA), 4);
    ASSERT_TRUE(has_alpha(PixFmt::RGBA));
    tested_fmts.insert(PixFmt::RGBA);

    // Check for all formats were tested
    for (int i = static_cast<int>(PixFmt::Undefined); i <= static_cast<int>(PixFmt::EndOf); ++i)
        ASSERT_TRUE(tested_fmts.contains(static_cast<PixFmt>(i)));
}

TEST(Frame, invalid_frame_by_default_ctor)
{
    Frame frame;

    ASSERT_FALSE(frame.data());
    ASSERT_FALSE(frame.is_valid());
    ASSERT_EQ(frame.bytesize(), 0);
    ASSERT_EQ(frame.bpp(), 0);
}

TEST(Frame, empty_frame_construction_and_destruction_with_default_deleter)
{
    Frame frame({100, 100}, PixFmt::GRAY);
    ASSERT_TRUE(frame.is_valid());
}

TEST(Frame, frame_opening)
{
    EXPECT_THROW(video::utils::open_file(TestDataProvider::frame_path(), PixFmt::Undefined), std::exception);

    auto frame = video::utils::open_file(TestDataProvider::frame_path(), TestDataProvider::pixel_format());

    ASSERT_TRUE(frame.is_valid());

    ASSERT_EQ(frame.size, TestDataProvider::frame_size());
    ASSERT_EQ(frame.pix_fmt, TestDataProvider::pixel_format());
    ASSERT_EQ(frame.bytesize(), TestDataProvider::frame_bytesize());
}

TEST_F(FrameTest, frames_copy_and_swap)
{
    auto& file_frame_orig = frame;
    Frame empty_frame_orig({100, 100}, PixFmt::GRAY);

    auto file_frame = file_frame_orig;
    auto empty_frame = empty_frame_orig;

    ASSERT_EQ(file_frame_orig, file_frame);
    ASSERT_EQ(empty_frame_orig, empty_frame);

    swap(file_frame, empty_frame);

    ASSERT_EQ(file_frame_orig, empty_frame);
    ASSERT_EQ(empty_frame_orig, file_frame);
}

TEST_F(FrameTest, frame_creation_shallow)
{
    auto copied = Frame::create(frame.size, frame.stride, frame.pix_fmt, frame.data(), Frame::empty_deleter);

    ASSERT_TRUE(copied.is_valid());

    ASSERT_EQ(frame.size, copied.size);
    ASSERT_EQ(frame.stride, copied.stride);
    ASSERT_EQ(frame.pix_fmt, copied.pix_fmt);

    ASSERT_EQ((void*)frame.data(), (void*)copied.data());
}

TEST_F(FrameTest, frame_deep_creation)
{
    auto copied = Frame::create_deep(frame.size, frame.stride, frame.pix_fmt, frame.data());

    ASSERT_TRUE(copied.is_valid());

    ASSERT_EQ(frame.size, copied.size);
    ASSERT_EQ(frame.stride, copied.stride);
    ASSERT_EQ(frame.pix_fmt, copied.pix_fmt);

    ASSERT_NE((void*)frame.data(), (void*)copied.data());
}

TEST_F(FrameTest, frame_colorspace_conversion)
{
    auto gray = frame;
    video::utils::convert_colorspace(gray, PixFmt::GRAY);
    ASSERT_TRUE(gray.is_valid());
    ASSERT_EQ(gray.bytesize(), frame.bytesize() / get_channels_count(frame.pix_fmt));
}

TEST_F(FrameTest, frame_saving)
{
    video::utils::save_to_file(frame, TestDataProvider::saved_frame_path());

    std::filesystem::path path(TestDataProvider::saved_frame_path());
    ASSERT_TRUE(std::filesystem::is_regular_file(path));
    ASSERT_TRUE(std::filesystem::remove(path));
}