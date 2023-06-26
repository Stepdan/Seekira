#pragma once

#include <core/base/interfaces/serializable.hpp>

#include <proc/interfaces/face_engine_user.hpp>

#include <filesystem>

namespace step::proc {

class PersonHolder : public IFaceEngineUser
{
public:
    using PersonId = std::string;

    struct Initializer : public ISerializable
    {
        PersonId person_id;
        std::filesystem::path path;

        void deserialize(const ObjectPtrJSON& container) override;
    };

public:
    PersonHolder();
    PersonHolder(Initializer&& init, const ConnId& conn_id);
    PersonHolder(const Initializer& init, const ConnId& conn_id);

    PersonId get_person_id() const noexcept { return m_id; }

    FaceMatchStatus compare(const FacePtr& face) const;

private:
    void load_from_dir(const std::filesystem::path& path);

private:
    PersonId m_id;
    std::filesystem::path m_path;
    Faces m_faces;
};

}  // namespace step::proc