#pragma once

#include <video/frame/interfaces/frame.hpp>

#include <opencv2/core/mat.hpp>

namespace step::video::utils {

// Non-owned Mat
cv::Mat to_mat(Frame& frame);  // TODO: rename to create_mat_view()

// Owned Mat
cv::Mat to_mat_deep(Frame& frame);  // TODO: rename to create_mat()

int get_colorspace_convert_id(PixFmt from, PixFmt to);

int get_cv_data_type(PixFmt);

}  // namespace step::video::utils