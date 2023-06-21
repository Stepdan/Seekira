#pragma once

#include "face_engine.hpp"

#include <core/base/interfaces/connector.hpp>

#include <memory>

namespace step::proc {

class IFaceEngineUser : public IConnectionObject
{
public:
    virtual ~IFaceEngineUser() = default;

    virtual void set_face_engine(const std::weak_ptr<IFaceEngine>& face_engine) { m_face_engine = face_engine; }

protected:
    std::shared_ptr<IFaceEngine> get_face_engine(bool required = true) const
    {
        const auto face_engine = m_face_engine.lock();
        if (!face_engine && required)
            STEP_ASSERT(!"IFaceEngineUser: face_engine is nullptr!");

        return face_engine;
    }

protected:
    std::weak_ptr<IFaceEngine> m_face_engine;
};

}  // namespace step::proc