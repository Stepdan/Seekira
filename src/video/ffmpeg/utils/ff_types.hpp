#pragma once

#include <cstdint>

namespace step::video::ff {

using TimeFF = int64_t;
using TimestampFF = int64_t;
using StreamId = int;

/* clang-format off */
constexpr TimeFF AV_MILLISECOND = 1000;
constexpr TimeFF AV_SECOND      = 1000 * AV_MILLISECOND;
constexpr TimeFF AV_MINUTE      = 60 * AV_SECOND;
constexpr TimeFF AV_HOUR        = 60 * AV_MINUTE;
constexpr TimeFF MAX_DURATION   = (TimeFF)(60 * 60 * 24 * 30) * AV_SECOND;
/* clang-format on */

enum class MediaType
{
    Undefined,
    Video,
    Audio,
};

}  // namespace step::video::ff