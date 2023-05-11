#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace step::video::ff {

class FFDecoderVideo
{
private:
    int m_stream_index;
};

}  // namespace step::video::ff