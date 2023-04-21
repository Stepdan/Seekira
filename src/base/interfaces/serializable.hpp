#pragma once

#include <base/utils/json/json_utils.hpp>
#include <base/utils/exception/throw_utils.hpp>

namespace step {

class ISerializable
{
public:
    virtual ~ISerializable() = default;

    virtual void serialize(ArrayPtrJSON& container)
    {
        utils::throw_runtime_with_log("Serialize to ArrayPtrJSON is not implemented!");
    }

    virtual void serialize(ObjectPtrJSON& container)
    {
        utils::throw_runtime_with_log("Serialize to ObjectPtrJSON is not implemented!");
    }

    virtual void deserialize(const ArrayPtrJSON& container)
    {
        utils::throw_runtime_with_log("Deserialize from ArrayPtrJSON is not implemented!");
    }

    virtual void deserialize(const ObjectPtrJSON& container)
    {
        utils::throw_runtime_with_log("Deserialize from ObjectPtrJSON is not implemented!");
    }
};

}  // namespace step