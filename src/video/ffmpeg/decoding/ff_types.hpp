#pragma once

#include <cstdint>

namespace step::video::ff {

using TimeFF = int64_t;
using TimestampFF = int64_t;
using StreamId = int;

enum class MediaType
{
    Undefined,
    Video,
    Audio,
    SubtitleVideo,
    SubtitleText
};

}  // namespace step::video::ff