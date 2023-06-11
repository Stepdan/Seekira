#include "reader_event.hpp"

#include <core/base/utils/find_pair.hpp>
#include <core/base/utils/string_utils.hpp>

#include <video/ffmpeg/interfaces/reader.hpp>

#include <string_view>
#include <utility>

namespace {

/* clang-format off */

constexpr std::pair<step::video::ff::ReaderMode, std::string_view> g_reader_modes[] = {
    { step::video::ff::ReaderMode::KeyFrame , "KeyFrame"    },
    { step::video::ff::ReaderMode::All      , "All"         },
};

constexpr std::pair<step::video::ff::ReaderState, std::string_view> g_reader_statutes[] = {
    { step::video::ff::ReaderState::EndOfFile           , "EndOfFile"           },
    { step::video::ff::ReaderState::Error               , "Error"               },
    { step::video::ff::ReaderState::Paused              , "Paused"              },
    { step::video::ff::ReaderState::Reading             , "Reading"             },
    { step::video::ff::ReaderState::Stopped             , "Stopped"             },
    { step::video::ff::ReaderState::Error               , "Error"               },
};

constexpr std::pair<step::video::ff::ReaderEvent::Type, std::string_view> g_reader_events[] = {
    { step::video::ff::ReaderEvent::Type::Pause             , "Pause"           },
    { step::video::ff::ReaderEvent::Type::Play              , "Play"            },
    { step::video::ff::ReaderEvent::Type::RewindBackward    , "RewindBackward"  },
    { step::video::ff::ReaderEvent::Type::RewindForward     , "RewindForward"   },
    { step::video::ff::ReaderEvent::Type::SetPosition       , "SetPosition"     },
    { step::video::ff::ReaderEvent::Type::StepBackward      , "StepBackward"    },
    { step::video::ff::ReaderEvent::Type::StepForward       , "StepForward"     },
    { step::video::ff::ReaderEvent::Type::Stop              , "Stop"            },
};

/* clang-format on */

}  // namespace

// to_string
namespace step::utils {

template <>
std::string to_string(step::video::ff::ReaderMode mode)
{
    return find_by_type(mode, g_reader_modes);
}

template <>
std::string to_string(step::video::ff::ReaderState state)
{
    return find_by_type(state, g_reader_statutes);
}

template <>
std::string to_string(step::video::ff::ReaderEvent::Type type)
{
    return find_by_type(type, g_reader_events);
}

template <>
void from_string(step::video::ff::ReaderMode& mode, const std::string& str)
{
    find_by_str(str, mode, g_reader_modes);
}

template <>
void from_string(step::video::ff::ReaderState& state, const std::string& str)
{
    find_by_str(str, state, g_reader_statutes);
}

template <>
void from_string(step::video::ff::ReaderEvent::Type& type, const std::string& str)
{
    find_by_str(str, type, g_reader_events);
}

}  // namespace step::utils