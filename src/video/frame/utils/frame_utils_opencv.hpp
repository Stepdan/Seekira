#pragma once

#include <core/base/types/interpolation.hpp>
#include <core/base/types/color.hpp>

#include <video/frame/interfaces/frame.hpp>

#include <opencv2/core/mat.hpp>

namespace step::video::utils {

// Non-owned Mat
cv::Mat to_mat(Frame& frame);

// Owned Mat
cv::Mat to_mat_deep(Frame& frame);

// We should exactly know what pixel format inside
Frame from_mat(cv::Mat& mat, PixFmt fmt);
Frame from_mat_deep(cv::Mat& mat, PixFmt fmt);

int get_colorspace_convert_id(PixFmt from, PixFmt to);

int get_cv_data_type(PixFmt);

cv::Size get_cv_size(const FrameSize& frame_size);

int get_cv_interpolation(InterpolationType type);

}  // namespace step::video::utils