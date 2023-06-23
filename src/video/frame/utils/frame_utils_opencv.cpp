#include "frame_utils_opencv.hpp"

#include <core/base/utils/find_pair.hpp>
#include <core/base/utils/string_utils.hpp>
#include <core/exception/assert.hpp>

#include <opencv2/core/hal/interface.h>
#include <opencv2/imgproc.hpp>

#include <fmt/format.h>

namespace {

/* clang-format off */
const std::pair<std::pair<step::video::PixFmt, step::video::PixFmt>, int> colorspace_convert_ids[] = {
    { {step::video::PixFmt::GRAY , step::video::PixFmt::BGR   }   , cv::COLOR_GRAY2BGR    },
    { {step::video::PixFmt::GRAY , step::video::PixFmt::RGB   }   , cv::COLOR_GRAY2RGB    },
    { {step::video::PixFmt::GRAY , step::video::PixFmt::BGRA  }   , cv::COLOR_GRAY2BGRA   },
    { {step::video::PixFmt::GRAY , step::video::PixFmt::RGBA  }   , cv::COLOR_GRAY2RGBA   },

    { {step::video::PixFmt::BGR  , step::video::PixFmt::GRAY  }   , cv::COLOR_BGR2GRAY    },
    { {step::video::PixFmt::BGR  , step::video::PixFmt::BGRA  }   , cv::COLOR_BGR2BGRA    },
    { {step::video::PixFmt::BGR  , step::video::PixFmt::RGB   }   , cv::COLOR_BGR2RGB     },
    { {step::video::PixFmt::BGR  , step::video::PixFmt::RGBA  }   , cv::COLOR_BGR2RGBA    },

    { {step::video::PixFmt::RGB  , step::video::PixFmt::GRAY  }   , cv::COLOR_RGB2GRAY    },
    { {step::video::PixFmt::RGB  , step::video::PixFmt::RGBA  }   , cv::COLOR_RGB2RGBA    },
    { {step::video::PixFmt::RGB  , step::video::PixFmt::BGR   }   , cv::COLOR_RGB2BGR     },
    { {step::video::PixFmt::RGB  , step::video::PixFmt::BGRA  }   , cv::COLOR_RGB2BGRA    },

    { {step::video::PixFmt::RGBA , step::video::PixFmt::GRAY  }   , cv::COLOR_RGBA2GRAY   },
    { {step::video::PixFmt::RGBA , step::video::PixFmt::RGB   }   , cv::COLOR_RGBA2RGB    },
    { {step::video::PixFmt::RGBA , step::video::PixFmt::BGR   }   , cv::COLOR_RGBA2BGR    },
    { {step::video::PixFmt::RGBA , step::video::PixFmt::BGRA  }   , cv::COLOR_RGBA2BGRA   },

    { {step::video::PixFmt::BGRA , step::video::PixFmt::GRAY  }   , cv::COLOR_BGRA2GRAY   },
    { {step::video::PixFmt::BGRA , step::video::PixFmt::BGR   }   , cv::COLOR_BGRA2BGR    },
    { {step::video::PixFmt::BGRA , step::video::PixFmt::RGB   }   , cv::COLOR_BGRA2RGB    },
    { {step::video::PixFmt::BGRA , step::video::PixFmt::RGBA  }   , cv::COLOR_BGRA2RGBA   },

};

const std::pair<step::video::PixFmt, int> cv_data_types[] = {
    { step::video::PixFmt::GRAY  , CV_8UC1 },
    { step::video::PixFmt::BGR   , CV_8UC3 },
    { step::video::PixFmt::BGRA  , CV_8UC4 },
    { step::video::PixFmt::RGB   , CV_8UC3 },
    { step::video::PixFmt::RGBA  , CV_8UC4 },
};

/* clang-format on */
}  // namespace

namespace step::video::utils {

cv::Mat to_mat(Frame& frame)
{
    return cv::Mat(frame.size.height, frame.size.width, utils::get_cv_data_type(frame.pix_fmt), frame.data(),
                   frame.stride);
}

cv::Mat to_mat_deep(Frame& frame) { return to_mat(frame).clone(); }

Frame from_mat(cv::Mat& mat, PixFmt fmt)
{
    return Frame::create(FrameSize(mat.cols, mat.rows), mat.step, fmt, mat.data);
}

Frame from_mat_deep(cv::Mat& mat, PixFmt fmt)
{
    return Frame::create_deep(FrameSize(mat.cols, mat.rows), mat.step, fmt, mat.data);
}

int get_colorspace_convert_id(PixFmt from, PixFmt to)
{
    if (auto it = step::utils::find_pair_iterator_by_first(colorspace_convert_ids, {from, to});
        it != std::cend(colorspace_convert_ids))
        return it->second;
    else
        STEP_THROW_RUNTIME("get_colorspace_convert_id(): No OpenCV colorspace for pix fmts: {} -> {}", from, to);
}

int get_cv_data_type(PixFmt fmt)
{
    if (auto it = step::utils::find_pair_iterator_by_first(cv_data_types, fmt); it != std::cend(cv_data_types))
        return it->second;
    else
        STEP_THROW_RUNTIME("get_cv_data_type(): No OpenCV data type for pix fmt: {}", fmt);
}

cv::Size get_cv_size(const FrameSize& frame_size) { return cv::Size(frame_size.width, frame_size.height); }

int get_cv_interpolation(InterpolationType type)
{
    switch (type)
    {
        case InterpolationType::Area:
            return cv::INTER_AREA;
        case InterpolationType::Cubic:
            return cv::INTER_CUBIC;
        case InterpolationType::Linear:
            return cv::INTER_LINEAR;
        default:
            STEP_UNDEFINED("Invalid InterpolationType");
    }

    return -1;
}

}  // namespace step::video::utils
