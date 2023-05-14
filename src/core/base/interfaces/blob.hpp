#pragma once

#include "serializable.hpp"

namespace step {

class IBlob : public ISerializable
{
protected:
    virtual ~IBlob() = default;

public:
    virtual uint8_t* data() = 0;
    virtual const uint8_t* data() const = 0;
    virtual size_t size() const = 0;
    virtual size_t real_size() const = 0;
};

}  // namespace step
