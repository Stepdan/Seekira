#include "drawer_node.hpp"

#include <core/base/types/config_fields.hpp>
#include <core/task/settings_factory.hpp>
#include <core/task/task_factory.hpp>

#include <proc/interfaces/detector_interface.hpp>
#include <proc/interfaces/face.hpp>

#include <proc/drawer/drawer.hpp>

namespace step::proc {

const std::string DrawerNodeSettings::SETTINGS_ID = "DrawerNodeSettings";

std::shared_ptr<task::BaseSettings> create_drawer_node_settings(const ObjectPtrJSON& cfg)
{
    return std::make_shared<DrawerNodeSettings>(cfg);
}

void DrawerNodeSettings::deserialize(const ObjectPtrJSON& container)
{
    auto drawer_settings_json = json::opt_object(container, CFG_FLD::DRAWER_SETTINGS);

    if (drawer_settings_json)
        m_drawer_settings = SettingsDrawer(drawer_settings_json);
    else
        m_drawer_settings = SettingsDrawer();
}

}  // namespace step::proc

namespace step::proc {

class DrawerPipelineNode : public PipelineNodeTask<video::Frame, DrawerNodeSettings>
{
public:
    DrawerPipelineNode(const std::shared_ptr<task::BaseSettings>& settings)
    {
        set_settings(*settings);
        m_drawer = std::make_unique<Drawer>(m_typed_settings.get_drawer_settings());
    }

    void process(PipelineDataPtr<video::Frame> pipeline_data) override { draw_faces(pipeline_data); }

private:
    void draw_faces(PipelineDataPtr<video::Frame> pipeline_data)
    {
        const auto process = [this, &pipeline_data](const MetaStorage& storage) {
            auto faces = storage.get_attachment<Faces>(CFG_FLD::FACES);
            if (faces.has_value())
            {
                for (const auto& face : faces.value())
                    m_drawer->draw(pipeline_data->data, face);
            }
        };

        auto face_detection_result =
            pipeline_data->storage.get_attachment<DetectionResult>(CFG_FLD::FACE_DETECTION_RESULT);

        // Смотрим, есть ли результаты детекции лиц
        if (face_detection_result.has_value())
            process(face_detection_result.value().data());

        // Вдруг есть данные в главном хранилище
        process(pipeline_data->storage);
    }

private:
    std::unique_ptr<proc::Drawer> m_drawer;
};

std::unique_ptr<task::IAbstractTask> create_drawer_node(const std::shared_ptr<task::BaseSettings>& settings)
{
    return std::make_unique<DrawerPipelineNode>(settings);
}

}  // namespace step::proc