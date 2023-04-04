#include "object.hpp"

#include <core/utils/json_utils.hpp>

namespace rvision {

// Object
void serialize(const Object& object, ObjectPtrJSON& container)
{
    auto pose_json = json::make_object_json_ptr();
    serialize(object.pose, pose_json);

    json::set(container, "id", object.id);
    json::set(container, "pose", pose_json);
}

void deserialize(const ObjectPtrJSON& container, Object& object)
{
    auto pose_json = json::get_object(container, "pose");
    deserialize(pose_json, object.pose);
    object.id = json::get<std::string>(container, "id");
}

// ScoredObject
void serialize(const ScoredObject& object, ObjectPtrJSON& container)
{
    serialize(object.get_object(), container);
    json::set(container, "score", object.score);
}

void deserialize(const ObjectPtrJSON& container, ScoredObject& object)
{
    deserialize(container, object);
    object.score = json::get<double>(container, "score");
}

}  // namespace rvision
