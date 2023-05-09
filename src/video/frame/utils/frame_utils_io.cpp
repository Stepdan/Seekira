#include "frame_utils.hpp"
#include "frame_utils_opencv.hpp"

#include <core/exception/assert.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace step::video::utils {

Frame open_file(const std::string& path, PixFmt fmt /*= PixFmt::BGR*/)
{
    int cv_open_flags;
    constexpr int NOOP = std::numeric_limits<int>::min();
    int cv_cvt_color_flags = NOOP;
    switch (fmt)
    {
        case PixFmt::GRAY:
            cv_open_flags = cv::IMREAD_GRAYSCALE;
            break;

        case PixFmt::RGB: {
            cv_cvt_color_flags = cv::COLOR_BGR2RGB;
            [[fallthrough]];
        }
        case PixFmt::BGR:
            cv_open_flags = cv::IMREAD_COLOR;
            break;

        case PixFmt::RGBA: {
            cv_cvt_color_flags = cv::COLOR_BGRA2RGBA;
            [[fallthrough]];
        }
        case PixFmt::BGRA:
            cv_open_flags = cv::IMREAD_UNCHANGED;
            break;
        default:
            STEP_THROW_RUNTIME("Invalid PixFmt for open file: {}", fmt);
    }

    auto mat = cv::imread(path, cv_open_flags);
    if (cv_cvt_color_flags != NOOP)
    {
        cv::cvtColor(mat, mat, cv_cvt_color_flags);
    }
    return Frame::create_deep({static_cast<size_t>(mat.cols), static_cast<size_t>(mat.rows)}, mat.step, fmt,
                              static_cast<Frame::DataTypePtr>(mat.data));
}

void save_to_file(Frame& frame, const std::filesystem::path& path)
{
    auto mat = to_mat(frame);
    cv::imwrite(path.string(), mat);
}

}  // namespace step::video::utils