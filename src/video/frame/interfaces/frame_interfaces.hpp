#pragma once

#include "frame.hpp"

#include <core/exception/assert.hpp>

namespace step::video {

class IFrameSourceObserver
{
public:
    virtual ~IFrameSourceObserver() = default;

    virtual void process_frame(FramePtr) { STEP_UNDEFINED("process_frame is undefined!"); }
};

class IFrameSource
{
public:
    virtual ~IFrameSource() = default;

    virtual void register_observer(IFrameSourceObserver* observer) = 0;
    virtual void unregister_observer(IFrameSourceObserver* observer) = 0;
};

}  // namespace step::video