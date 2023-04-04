#include "frame_utils_opencv.hpp"

#include <core/utils/find_pair.hpp>
#include <core/utils/string_utils.hpp>
#include <core/utils/throw_utils.hpp>

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

    // BayerRG<->BayerBG (RGGB), BayerBG<->BayerRG (BGGR), BayerGB<->BayerGR (GBRG), BayerGR<->BayerGB (GRBG)
    // because of OpenCV naming inconsistency in Bayer pattern
    // https://github.com/opencv/opencv/issues/19629
    // https://docs.opencv.org/master/de/d25/imgproc_color_conversions.html (see Bayer → RGB example)
    { {step::video::PixFmt::BayerGR , step::video::PixFmt::BGR   }   , cv::COLOR_BayerGB2BGR_EA   },
    { {step::video::PixFmt::BayerGB , step::video::PixFmt::BGR   }   , cv::COLOR_BayerGR2BGR_EA   },
    { {step::video::PixFmt::BayerRG , step::video::PixFmt::BGR   }   , cv::COLOR_BayerBG2BGR_EA   },
    { {step::video::PixFmt::BayerBG , step::video::PixFmt::BGR   }   , cv::COLOR_BayerRG2BGR_EA   },

    { {step::video::PixFmt::BayerGR , step::video::PixFmt::RGB   }   , cv::COLOR_BayerGB2RGB_EA   },
    { {step::video::PixFmt::BayerGB , step::video::PixFmt::RGB   }   , cv::COLOR_BayerGR2RGB_EA   },
    { {step::video::PixFmt::BayerRG , step::video::PixFmt::RGB   }   , cv::COLOR_BayerBG2RGB_EA   },
    { {step::video::PixFmt::BayerBG , step::video::PixFmt::RGB   }   , cv::COLOR_BayerRG2RGB_EA   },

    // Note: even those these are here, they may be very wrong
    { {step::video::PixFmt::BayerGR , step::video::PixFmt::BGRA   }   , cv::COLOR_BayerGB2BGRA   },
    { {step::video::PixFmt::BayerGB , step::video::PixFmt::BGRA   }   , cv::COLOR_BayerGR2BGRA   },
    { {step::video::PixFmt::BayerRG , step::video::PixFmt::BGRA   }   , cv::COLOR_BayerBG2BGRA   },
    { {step::video::PixFmt::BayerBG , step::video::PixFmt::BGRA   }   , cv::COLOR_BayerRG2BGRA   },

    { {step::video::PixFmt::BayerGR , step::video::PixFmt::RGBA   }   , cv::COLOR_BayerGB2RGBA   },
    { {step::video::PixFmt::BayerGB , step::video::PixFmt::RGBA   }   , cv::COLOR_BayerGR2RGBA   },
    { {step::video::PixFmt::BayerRG , step::video::PixFmt::RGBA   }   , cv::COLOR_BayerBG2RGBA   },
    { {step::video::PixFmt::BayerBG , step::video::PixFmt::RGBA   }   , cv::COLOR_BayerRG2RGBA   },

};

const std::pair<step::video::PixFmt, int> cv_data_types[] = {
    { step::video::PixFmt::GRAY  , CV_8UC1 },
    { step::video::PixFmt::BGR   , CV_8UC3 },
    { step::video::PixFmt::BGRA  , CV_8UC4 },
    { step::video::PixFmt::RGB   , CV_8UC3 },
    { step::video::PixFmt::RGBA  , CV_8UC4 },

    { step::video::PixFmt::BayerGR, CV_8UC1 },
    { step::video::PixFmt::BayerGB, CV_8UC1 },
    { step::video::PixFmt::BayerRG, CV_8UC1 },
    { step::video::PixFmt::BayerBG, CV_8UC1 },
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

int get_colorspace_convert_id(PixFmt from, PixFmt to)
{
    if (auto it = step::utils::find_pair_iterator_by_first(colorspace_convert_ids, {from, to});
        it != std::cend(colorspace_convert_ids))
        return it->second;
    else
        step::utils::throw_runtime_with_log(
            fmt::format("get_colorspace_convert_id(): No OpenCV colorspace for pix fmts: {} -> {}", from, to));
}

int get_cv_data_type(PixFmt fmt)
{
    if (auto it = step::utils::find_pair_iterator_by_first(cv_data_types, fmt); it != std::cend(cv_data_types))
        return it->second;
    else
        step::utils::throw_runtime_with_log(
            fmt::format("get_cv_data_type(): No OpenCV data type for pix fmt: {}", fmt));
}

}  // namespace step::video::utils
