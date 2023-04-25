#include "camera_extrinsics.hpp"

namespace step::video {

bool CameraExtrinsics::operator==(const CameraExtrinsics& rhs) const noexcept { return pose == rhs.pose; }

void CameraExtrinsics::serialize(ObjectPtrJSON& container) { pose.serialize(container); }

void CameraExtrinsics::deserialize(const ObjectPtrJSON& container) { pose.deserialize(container); }

}  // namespace step::video
