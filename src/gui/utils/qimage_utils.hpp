#pragma once

#include <video/frame/interfaces/frame.hpp>

#include <opencv2/core.hpp>

#include <QImage>
#include <QPixmap>

namespace step::gui::utils {

/*
    BGR by default.
    Don't change pre-defined values from QImage2cvMat, etc.
*/

video::Frame qimage_to_frame(const QImage& image);
video::Frame qimage_to_frame_deep(const QImage& image);

QImage frame_to_qimage(video::Frame& frame);
QImage frame_to_qimage_deep(video::Frame& frame);

video::Frame qpixmap_to_frame(const QPixmap& image);
QPixmap frame_to_qpixmap(video::Frame& frame);

}  // namespace step::gui::utils