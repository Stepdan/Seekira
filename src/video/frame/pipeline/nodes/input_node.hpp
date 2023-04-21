#pragma once

#include <video/frame/pipeline/node.hpp>

namespace step::video::pipeline {

class InputNodeSettings : public task::BaseSettings
{
public:
    TASK_SETTINGS(InputNodeSettings);

    InputNodeSettings() = default;

    bool operator==(const InputNodeSettings& rhs) const noexcept { return true; }
    bool operator!=(const InputNodeSettings& rhs) const noexcept { return !(*this == rhs); }
};

class InputPipelineNode : public FramePipelineNode<InputNodeSettings>
{
    using DataType = FramePipelineNode<InputNodeSettings>::DataType;

public:
    InputPipelineNode() = default;

    void process(DataType) override;
};

}  // namespace step::video::pipeline