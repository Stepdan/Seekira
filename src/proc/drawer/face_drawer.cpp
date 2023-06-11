#include "drawer.hpp"

#include <proc/interfaces/face.hpp>

#include <core/exception/assert.hpp>

#include <video/frame/utils/frame_utils_opencv.hpp>

#include <opencv2/imgproc.hpp>

namespace step::proc {

void draw(video::Frame& frame, const FacePtr& face)
{
    STEP_ASSERT(face, "Invalid face to draw!");

    const auto rect = face->get_rect();

    auto mat = video::utils::to_mat(frame);
    cv::rectangle(mat, cv::Rect(rect.p0.x, rect.p0.y, std::abs(rect.p1.x - rect.p0.x), std::abs(rect.p1.y - rect.p0.y)),
                  cv::Scalar(0, 255, 0), 2);
}

}  // namespace step::proc