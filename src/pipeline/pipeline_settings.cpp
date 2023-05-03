#include "pipeline_settings.hpp"

#include <base/types/config_fields.hpp>

namespace step::pipeline {

PipelineSettings::PipelineSettings(const ObjectPtrJSON& config) { deserialize(config); }

void PipelineSettings::deserialize(const ObjectPtrJSON& config)
{
    name = json::get<std::string>(config, CFG_FLD::NAME);
}

}  // namespace step::pipeline