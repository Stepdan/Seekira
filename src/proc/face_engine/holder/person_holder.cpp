#include "person_holder.hpp"

#include <core/log/log.hpp>

#include <core/base/interfaces/connector.hpp>

#include <video/frame/utils/frame_utils.hpp>

namespace {

constexpr double VALID_THRESHOLD = 0.75;

}

namespace step::proc {

PersonHolder::PersonHolder() {}

PersonHolder::PersonHolder(const ConnId& face_user_conn_id)
{
    set_conn_id(face_user_conn_id);
    Connector::connect(this);
}

void PersonHolder::load_from_dir(const std::filesystem::path& path)
{
    STEP_ASSERT(std::filesystem::is_directory(path), "Load face from invalid dir: {}", path.string());
    STEP_LOG(L_INFO, "Load face from dir: {}", path.string());

    auto face_engine = get_face_engine(true);

    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (std::filesystem::is_directory(entry))  // skip directories
            continue;

        const auto& entry_path = entry.path();

        auto frame = video::utils::open_file(entry_path);
        auto faces_from_frame = face_engine->detect(frame);
        if (faces_from_frame.size() != 1)
        {
            STEP_LOG(L_ERROR, "Faces count is not 1,  on image {}, skip frame", entry_path.string());
            continue;
        }

        face_engine->recognize(faces_from_frame.front());
        m_faces.push_back(faces_from_frame.front());
    }

    // Проверим, что все лица принадлежат одному человеку
    for (size_t i = 0; i < m_faces.size() - 1; ++i)
        for (size_t j = i + 1; j < m_faces.size(); ++j)
        {
            auto is_equal = face_engine->compare(m_faces[i], m_faces[j]);
            if (!is_equal)
                STEP_LOG(L_WARN, "There are non equal faces in PersonHolder");
        }
}

bool PersonHolder::compare(const FacePtr& face) const
{
    auto face_engine = get_face_engine(true);

    int valid_counter = 0;
    for (size_t i = 0; i < m_faces.size(); ++i)
    {
        if (face_engine->compare(m_faces[i], face))
            ++valid_counter;
    }

    const auto valid_percentage = 1.0 * valid_counter / m_faces.size();

    return valid_percentage > VALID_THRESHOLD;
}

}  // namespace step::proc