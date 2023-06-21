#include "face_engine_controller.hpp"

#include <core/base/types/config_fields.hpp>

#include <proc/face_engine/face_engine_factory.hpp>

namespace step::proc {

IFaceEngineController::IFaceEngineController(const ObjectPtrJSON& cfg) : IConnectionSource() { deserialize(cfg); }

void IFaceEngineController::connect(IConnectionObject* obj)
{
    auto face_engine_user = dynamic_cast<IFaceEngineUser*>(obj);
    STEP_ASSERT(face_engine_user, "Can't convert connectable to IFaceEngineUser");
    add_user(face_engine_user);
}

void IFaceEngineController::disconnect(IConnectionObject* obj) {}

void IFaceEngineController::deserialize(const ObjectPtrJSON& container)
{
    set_conn_id(json::get<std::string>(container, CFG_FLD::FACE_ENGINE_CONNECTION_ID));

    IFaceEngine::Initializer init;
    init.deserialize(json::get_object(container, CFG_FLD::FACE_ENGINE_INIT));
    set_face_engine(create_face_engine(std::move(init)));
}

}  // namespace step::proc