#pragma once

#include <opencv2/opencv.hpp>

namespace step::proc {

struct YoloObject
{
    cv::Rect2f rect;
    float prob;
    int label;
};

}  // namespace step::proc