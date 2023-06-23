#include "resizer_node.hpp"

#include <core/base/types/config_fields.hpp>
#include <core/task/settings_factory.hpp>
#include <core/task/task_factory.hpp>

#include <proc/interfaces/effect_interface.hpp>

namespace step::proc {

const std::string ResizerNodeSettings::SETTINGS_ID = "ResizerNodeSettings";

std::shared_ptr<task::BaseSettings> create_resizer_node_settings(const ObjectPtrJSON& cfg)
{
    return std::make_shared<ResizerNodeSettings>(cfg);
}

void ResizerNodeSettings::deserialize(const ObjectPtrJSON& container)
{
    auto resizer_settings_json = json::get_object(container, CFG_FLD::SETTINGS);
    m_settings = CREATE_SETTINGS(resizer_settings_json);
}

}  // namespace step::proc

namespace step::proc {

class ResizerPipelineNode : public PipelineNodeTask<video::Frame, ResizerNodeSettings>
{
public:
    ResizerPipelineNode(const std::shared_ptr<task::BaseSettings>& settings)
    {
        set_settings(*settings);

        auto resizer_settings_base = m_typed_settings.get_resizer_settings_base();
        STEP_ASSERT(resizer_settings_base, "Invalid resizer_settings_base");
        m_resizer = IEffect::from_abstract(CREATE_TASK_UNIQUE(resizer_settings_base));
    }

    void process(PipelineDataPtr<video::Frame> pipeline_data) override
    {
        EffectMultiInput input;
        input.emplace_back(std::move(pipeline_data->data));

        pipeline_data->data = std::move(m_resizer->process(input));
    }

private:
    std::unique_ptr<IEffect> m_resizer;
};

std::unique_ptr<task::IAbstractTask> create_resizer_node(const std::shared_ptr<task::BaseSettings>& settings)
{
    return std::make_unique<ResizerPipelineNode>(settings);
}

}  // namespace step::proc