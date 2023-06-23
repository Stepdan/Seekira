#include "frame_size.hpp"

#include <core/base/types/config_fields.hpp>

namespace step::video {

void FrameSize::deserialize(const ObjectPtrJSON& cfg)
{
    width = json::get<size_t>(cfg, CFG_FLD::WIDTH);
    height = json::get<size_t>(cfg, CFG_FLD::HEIGHT);
}

}  // namespace step::video