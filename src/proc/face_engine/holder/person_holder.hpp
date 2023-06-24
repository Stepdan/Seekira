#pragma once

#include <proc/interfaces/face_engine_user.hpp>

#include <filesystem>

namespace step::proc {

class PersonHolder : public IFaceEngineUser
{
public:
    PersonHolder();
    PersonHolder(const ConnId& face_user_conn_id);

    void load_from_dir(const std::filesystem::path& path);

    bool compare(const FacePtr& face) const;

private:
    Faces m_faces;
};

}  // namespace step::proc