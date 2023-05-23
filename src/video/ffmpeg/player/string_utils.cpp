#include "player_types.hpp"

#include <core/base/utils/find_pair.hpp>
#include <core/base/utils/string_utils.hpp>

#include <string_view>
#include <utility>

namespace {

/* clang-format off */

constexpr std::pair<step::video::ff::MediaStatus, std::string_view> g_media_statutes[] = {
    { step::video::ff::MediaStatus::BufferedMedia   , "BufferedMedia"   },
    { step::video::ff::MediaStatus::BufferingMedia  , "BufferingMedia"  },
    { step::video::ff::MediaStatus::EndOfMedia      , "EndOfMedia"      },
    { step::video::ff::MediaStatus::InvalidMedia    , "InvalidMedia"    },
    { step::video::ff::MediaStatus::LoadedMedia     , "LoadedMedia"     },
    { step::video::ff::MediaStatus::LoadingMedia    , "LoadingMedia"    },
    { step::video::ff::MediaStatus::NoMedia         , "NoMedia"         },
    { step::video::ff::MediaStatus::StalledMedia    , "StalledMedia"    },
};

constexpr std::pair<step::video::ff::PlayingState, std::string_view> g_playing_states[] = {
    { step::video::ff::PlayingState::Error  , "Error"   },
    { step::video::ff::PlayingState::Paused , "Paused"  },
    { step::video::ff::PlayingState::Playing, "Playing" },
    { step::video::ff::PlayingState::Seeking, "Seeking" },
    { step::video::ff::PlayingState::Stopped, "Stopped" },
};

/* clang-format on */

}  // namespace

// to_string
namespace step::utils {

template <>
std::string to_string(step::video::ff::MediaStatus status)
{
    return find_by_type(status, g_media_statutes);
}

template <>
std::string to_string(step::video::ff::PlayingState state)
{
    return find_by_type(state, g_playing_states);
}

template <>
void from_string(step::video::ff::MediaStatus& status, const std::string& str)
{
    find_by_str(str, status, g_media_statutes);
}

template <>
void from_string(step::video::ff::PlayingState& state, const std::string& str)
{
    find_by_str(str, state, g_playing_states);
}

}  // namespace step::utils