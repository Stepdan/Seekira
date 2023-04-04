#pragma once

#include "pose.hpp"

namespace rvision {

struct Object
{
public:
    Object() = default;
    Object(const std::string& _id, const Pose& _pose = Pose()) : id(_id), pose(_pose) {}

public:
    std::string id;
    Pose pose;
};

void serialize(const Object&, ObjectPtrJSON&);

void deserialize(const ObjectPtrJSON& container, Object&);

class ScoredObject : public Object
{
public:
    ScoredObject() = default;
    ScoredObject(const std::string& _id, const Pose& _pose = Pose(), double _score = 0.0)
        : Object(_id, _pose), score(_score)
    {
    }

    ScoredObject(const Object& _object, double _score = 0.0)
        : Object(_object), score(_score)
    {
    }

    const Object& get_object() const noexcept { return *this; }

public:
    double score;
};

void serialize(const ScoredObject&, ObjectPtrJSON&);

void deserialize(const ObjectPtrJSON& container, ScoredObject&);

}  // namespace rvision
