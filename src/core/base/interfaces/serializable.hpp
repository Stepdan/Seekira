#pragma once

#include <core/exception/assert.hpp>
#include <core/base/json/json_utils.hpp>

namespace step {

class ISerializable
{
public:
    virtual ~ISerializable() = default;

    virtual void serialize(ArrayPtrJSON& container)
    {
        STEP_THROW_RUNTIME("Serialize to ArrayPtrJSON is not implemented!");
    }

    virtual void serialize(ObjectPtrJSON& container)
    {
        STEP_THROW_RUNTIME("Serialize to ObjectPtrJSON is not implemented!");
    }

    virtual void deserialize(const ArrayPtrJSON& container)
    {
        STEP_THROW_RUNTIME("Deserialize from ArrayPtrJSON is not implemented!");
    }

    virtual void deserialize(const ObjectPtrJSON& container)
    {
        STEP_THROW_RUNTIME("Deserialize from ObjectPtrJSON is not implemented!");
    }
};

}  // namespace step