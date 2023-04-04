#include "pixel_format.hpp"

#include <core/utils/find_pair.hpp>
#include <core/utils/throw_utils.hpp>

#include <cassert>
#include <utility>

namespace {

/* clang-format off */

/*
    In case of usage 'to_string' - first pair with similar PixFmt in the list would be chosen.
*/
constexpr std::pair<step::video::PixFmt, std::string_view> pixel_formats[] = {
    { step::video::PixFmt::GRAY     , "GRAY"         },
    { step::video::PixFmt::GRAY     , "Grayscaled"   },
    { step::video::PixFmt::GRAY     , "Mono8"        }, // Basler

    { step::video::PixFmt::BGR      , "BGR"          },
    { step::video::PixFmt::BGR      , "BGR8Packed"   },
    { step::video::PixFmt::BGR      , "BGR8"   }, // Basler

    { step::video::PixFmt::BGRA     , "BGRA"         },
    { step::video::PixFmt::BGRA     , "BGRA8Packed"  },

    { step::video::PixFmt::RGB      , "RGB"          },
    { step::video::PixFmt::RGB      , "RGB8Packed"   },
    { step::video::PixFmt::RGB      , "RGB8"   }, // Basler

    { step::video::PixFmt::RGBA     , "RGBA"         },
    { step::video::PixFmt::RGBA     , "RGBA8Packed"  },

    { step::video::PixFmt::BayerGR  , "BayerGR"      },
    { step::video::PixFmt::BayerGR  , "BayerGR8"     }, // Basler

    { step::video::PixFmt::BayerGB  , "BayerGB"      },
    { step::video::PixFmt::BayerGB  , "BayerGB8"     }, // Basler

    { step::video::PixFmt::BayerRG  , "BayerRG"      },
    { step::video::PixFmt::BayerRG  , "BayerRG8"     }, // Basler

    { step::video::PixFmt::BayerBG  , "BayerBG"      },
    { step::video::PixFmt::BayerBG  , "BayerBG8"     }, // Basler
};

/* clang-format on */

}  // namespace

namespace step::video::utils {

size_t get_bpp(PixFmt fmt)
{
    switch (fmt)
    {
        case PixFmt::GRAY:
            return 8;
        case PixFmt::BGR:
            return 24;
        case PixFmt::BGRA:
            return 32;
        case PixFmt::RGB:
            return 24;
        case PixFmt::RGBA:
            return 32;
        case PixFmt::Undefined:
            [[fallthrough]];
        default:
            return 0;
    }

    return 0;
}

size_t get_channels_count(PixFmt fmt)
{
    switch (fmt)
    {
        case PixFmt::GRAY:
            return 1;
        case PixFmt::BGR:
            return 3;
        case PixFmt::BGRA:
            return 4;
        case PixFmt::RGB:
            return 3;
        case PixFmt::RGBA:
            return 4;
        case PixFmt::Undefined:
            [[fallthrough]];
        default:
            return 0;
    }

    return 0;
}

bool has_alpha(PixFmt fmt)
{
    switch (fmt)
    {
        case PixFmt::BGRA:
            [[fallthrough]];
        case PixFmt::RGBA:
            return true;
        default:
            return false;
    }

    return false;
}

}  // namespace step::video::utils

// to_string
namespace step::utils {

template <>
std::string to_string(video::PixFmt pixFmt)
{
    return find_by_type(pixFmt, pixel_formats);
}

template <>
std::vector<std::string> to_strings(video::PixFmt pixFmt)
{
    return find_all_by_type(pixFmt, pixel_formats);
}

template <>
void from_string(video::PixFmt& pixFmt, const std::string& str)
{
    find_by_str(str, pixFmt, pixel_formats);
}

}  // namespace step::utils
