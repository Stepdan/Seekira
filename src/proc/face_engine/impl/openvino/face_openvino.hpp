#pragma once

#include <proc/interfaces/face.hpp>

#include <thirdparty/tdv/api/Service.h>

namespace step::proc {

class FaceEngineTDV;

class FaceTDV : public BaseFace<api::Context>
{
    friend class FaceEngineTDV;

    static std::shared_ptr<FaceTDV> create_face() { return std::make_shared<FaceTDV>(); }

public:
    FaceTDV() = default;
};

}  // namespace step::proc