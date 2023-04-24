#pragma once

#include "meta_storage.hpp"

#include <base/utils/exception/assert.hpp>
#include <abstract_task/base_task.hpp>
#include <video/frame/interfaces/frame.hpp>

namespace step::video::pipeline {

struct FramePipelineDataType
{
    FramePtr frame;
    MetaStoragePtr storage;

    FramePipelineDataType clone()
    {
        STEP_ASSERT(frame && storage, "Invalid FramePipelineDataType!");

        FramePipelineDataType copy;
        copy.frame = std::make_shared<Frame>(*frame);
        copy.storage = std::make_shared<MetaStorage>(*storage);
        return copy;
    }
};

using IFramePipelineNode = task::ITask<FramePipelineDataType&>;

template <typename TSettings>
using FramePipelineNode = task::BaseTask<FramePipelineDataType&, TSettings>;

}  // namespace step::video::pipeline