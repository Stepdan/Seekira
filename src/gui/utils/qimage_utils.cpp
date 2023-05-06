#include "qimage_utils.hpp"

#include <base/utils/find_pair.hpp>
#include <base/utils/exception/throw_utils.hpp>
#include <video/frame/utils/frame_utils_opencv.hpp>

#include <opencv2/imgproc/imgproc.hpp>

#include <QSysInfo>

#include <fmt/format.h>

namespace {

/* clang-format off */

const std::pair<step::video::PixFmt, QImage::Format> g_qimageformat_to_pixfmt[] = {
    { step::video::PixFmt::GRAY, QImage::Format_Grayscale8             },
    { step::video::PixFmt::BGR , QImage::Format_BGR888                 },
    { step::video::PixFmt::RGB , QImage::Format_RGB888                 },
    { step::video::PixFmt::RGBA, QImage::Format_RGBA8888               },
};
/* clang-format on */

QImage::Format pix_fmt_to_qimage_fmt(step::video::PixFmt fmt)
{
    if (auto it = step::utils::find_pair_iterator_by_first(g_qimageformat_to_pixfmt, fmt);
        it != std::cend(g_qimageformat_to_pixfmt))
    {
        return it->second;
    }
    else
    {
        step::utils::throw_runtime_with_log(fmt::format("Invalid pix fmt {}!", fmt));
    }
}

step::video::PixFmt qimage_fmt_to_pix_fmt(QImage::Format fmt)
{
    if (auto it = step::utils::find_pair_iterator_by_second(g_qimageformat_to_pixfmt, fmt);
        it != std::cend(g_qimageformat_to_pixfmt))
    {
        return it->first;
    }
    else
    {
        step::utils::throw_runtime_with_log(fmt::format("Invalid qimage fmt {}!", fmt));
    }
}

cv::Mat qimage_to_mat(const QImage& img)
{
    if (img.isNull())
        return cv::Mat();

    return cv::Mat(img.height(), img.width(), CV_8UC(img.depth() / 8), (uchar*)img.bits(), img.bytesPerLine());
}

cv::Mat qimage_to_mat_deep(const QImage& img) { return qimage_to_mat(img).clone(); }

QImage mat_to_qimage(const cv::Mat& mat, QImage::Format fmt)
{
    return QImage(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), fmt);
}

QImage mat_to_qimage_deep(const cv::Mat& mat, QImage::Format fmt) { return mat_to_qimage(mat, fmt).copy(); }

QPixmap mat_to_qpixmap(const cv::Mat& mat, QImage::Format fmt) { return QPixmap::fromImage(mat_to_qimage(mat, fmt)); }

cv::Mat qpixmap_to_mat(const QPixmap& pixmap) { return qimage_to_mat(pixmap.toImage()); }

}  // namespace

namespace step::gui::utils {

/* clang-format off */
video::Frame qimage_to_frame(const QImage & image)
{
    auto pix_fmt = qimage_fmt_to_pix_fmt(image.format());
    auto mat = qimage_to_mat(image);
    return video::Frame::create({static_cast<size_t>(mat.cols), static_cast<size_t>(mat.rows)}, mat.step, pix_fmt,
                              static_cast<video::Frame::DataTypePtr>(mat.data), video::Frame::empty_deleter);
}

video::Frame qimage_to_frame_deep(const QImage& image)
{
    auto pix_fmt = qimage_fmt_to_pix_fmt(image.format());
    auto mat = qimage_to_mat(image);
    return video::Frame::create_deep({static_cast<size_t>(mat.cols), static_cast<size_t>(mat.rows)}, mat.step, pix_fmt,
                              static_cast<video::Frame::DataTypePtr>(mat.data));
}

QImage frame_to_qimage(video::Frame& frame)
{
    auto mat = video::utils::to_mat(frame);
    return mat_to_qimage(mat, pix_fmt_to_qimage_fmt(frame.pix_fmt));
}

QImage frame_to_qimage_deep(video::Frame& frame)
{
    auto mat = video::utils::to_mat(frame);
    return mat_to_qimage_deep(mat, pix_fmt_to_qimage_fmt(frame.pix_fmt));
}

video::Frame qpixmap_to_frame(const QPixmap& image)
{
    return qimage_to_frame(image.toImage());
}

QPixmap frame_to_qpixmap(video::Frame& frame)
{
    return QPixmap::fromImage(frame_to_qimage(frame));
}

/* clang-format on */

}  // namespace step::gui::utils