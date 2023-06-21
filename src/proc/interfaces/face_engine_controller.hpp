#pragma once

#include "face_engine_user.hpp"

#include <core/base/interfaces/connector.hpp>
#include <core/base/interfaces/user_controller.hpp>

namespace step::proc {

class IFaceEngineController : public IUserController<IFaceEngineUser>, public IConnectionSource, public ISerializable
{
public:
    IFaceEngineController(const ObjectPtrJSON& cfg);
    virtual ~IFaceEngineController() = default;

    virtual void set_face_engine(const std::shared_ptr<IFaceEngine>& face_engine)
    {
        m_face_engine = face_engine;
        for (auto& user : m_users)
            user->set_face_engine(face_engine);
    }

    virtual std::shared_ptr<IFaceEngine> get_face_engine() const { return m_face_engine; }

    void deserialize(const ObjectPtrJSON& container);

private:
    void add_controller_data_to_user(IFaceEngineUser* user) { user->set_face_engine(m_face_engine); }

    // IConnectionSource
private:
    void connect(IConnectionObject*) override;
    void disconnect(IConnectionObject*) override;

protected:
    std::shared_ptr<IFaceEngine> m_face_engine;
};

}  // namespace step::proc