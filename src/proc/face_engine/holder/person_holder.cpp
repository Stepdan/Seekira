#include "person_holder.hpp"

#include <core/log/log.hpp>

#include <core/base/interfaces/connector.hpp>
#include <core/base/types/config_fields.hpp>
#include <core/base/utils/type_utils.hpp>

#include <video/frame/utils/frame_utils.hpp>

namespace {

constexpr double VALID_THRESHOLD = 0.5;

}

namespace step::proc {

void PersonHolder::Initializer::deserialize(const ObjectPtrJSON& container)
{
    person_id = json::get<std::string>(container, CFG_FLD::ID);
    path = std::filesystem::path(json::get<std::string>(container, CFG_FLD::PATH));
}

PersonHolder::PersonHolder() {}

PersonHolder::PersonHolder(Initializer&& init, const ConnId& conn_id)
    : m_id(std::move(init.person_id)), m_path(std::move(init.path))
{
    set_conn_id(conn_id);
    Connector::connect(this);

    if (!m_path.empty())
        load_from_dir(m_path);
}

PersonHolder::PersonHolder(const Initializer& init, const ConnId& conn_id) : m_id(init.person_id), m_path(init.path)
{
    set_conn_id(conn_id);
    Connector::connect(this);

    if (!m_path.empty())
        load_from_dir(m_path);
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
            STEP_LOG(L_ERROR, "Faces count is not 1, on image {}, skip frame", entry_path.string());
            continue;
        }

        face_engine->recognize(faces_from_frame.front());
        m_faces.push_back(faces_from_frame.front());
    }

    // Проверим, что все лица принадлежат одному человеку
    for (size_t i = 0; i < m_faces.size() - 1; ++i)
        for (size_t j = i + 1; j < m_faces.size(); ++j)
        {
            auto match_result = face_engine->compare(m_faces[i], m_faces[j]);
            if (match_result.status != FaceMatchStatus::Matched)
                STEP_LOG(L_WARN, "There are non equal faces in PersonHolder");
        }
}

FaceMatchStatus PersonHolder::compare(const FacePtr& face) const
{
    face->set_match_status(FaceMatchStatus::Undefined);

    auto face_engine = get_face_engine(true);

    int valid_counter = 0;
    int invalid_counter = 0;
    int prob_counter = 0;
    std::vector<FaceMatchResult> results;
    for (size_t i = 0; i < m_faces.size(); ++i)
    {
        results.push_back(face_engine->compare(m_faces[i], face));

        const auto& status = results.back().status;

        switch (status)
        {
            case FaceMatchStatus::Matched:
                ++valid_counter;
                break;

            case FaceMatchStatus::NotMatched:
                ++invalid_counter;
                break;

            case FaceMatchStatus::Possible:
                ++prob_counter;
                break;

            default:
                STEP_ASSERT("Invalid face match status in FaceMatchResult!");
        }
    }

    const auto size = m_faces.size();
    double prob_percentage = 1.0 * prob_counter / size;
    double valid_percentage = 1.0 * valid_counter / size;
    double invalid_percentage = 1.0 * invalid_counter / size;

    FaceMatchStatus status{FaceMatchStatus::Undefined};
    if (valid_counter == 0 && prob_counter == 0)
        status = FaceMatchStatus::NotMatched;

    if (valid_counter == 0 && prob_counter != 0)
        status = FaceMatchStatus::Possible;

    if (valid_counter != 0)
    {
        // Посчитаем сумму весов результатов
        // Валидный - 1, возможный - 0.5
        if (step::utils::compare_ge((valid_counter + prob_counter / 2.0) / size, 0.5))
            status = FaceMatchStatus::Matched;
        else
            status = FaceMatchStatus::Possible;
    }

    face->set_match_status(status);

    STEP_LOG(L_INFO, "Face {} compared with person {}: v {}, i {}, p {}. Verdict: {}", face->get_rect(), m_id,
             valid_counter, invalid_counter, prob_counter, step::utils::to_string(status));

    return status;
}

}  // namespace step::proc