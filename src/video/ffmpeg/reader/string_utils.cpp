#include "reader_types.hpp"

#include <core/base/utils/find_pair.hpp>
#include <core/base/utils/string_utils.hpp>

#include <string_view>
#include <utility>

namespace {

/* clang-format off */

constexpr std::pair<step::video::ff::ReaderState, std::string_view> g_reader_statutes[] = {
    { step::video::ff::ReaderState::EndOfFile           , "EndOfFile"           },
    { step::video::ff::ReaderState::Error               , "Error"               },
    { step::video::ff::ReaderState::InvalidSeek         , "InvalidSeek"         },
    { step::video::ff::ReaderState::ReadingByRequest    , "ReadingByRequest"    },
    { step::video::ff::ReaderState::ReadingContiniously , "ReadingContiniously" },
    { step::video::ff::ReaderState::Ready               , "Ready"               },
    { step::video::ff::ReaderState::SuccessfulSeek      , "SuccessfulSeek"      },
    { step::video::ff::ReaderState::TryToSeek           , "TryToSeek"           },
};

constexpr std::pair<step::video::ff::ReadingMode, std::string_view> g_reading_modes[] = {
    { step::video::ff::ReadingMode::ByRequest       , "ByRequest"       },
    { step::video::ff::ReadingMode::Continuously    , "Continuously"    },
};

/* clang-format on */

}  // namespace

// to_string
namespace step::utils {

template <>
std::string to_string(step::video::ff::ReaderState state)
{
    return find_by_type(state, g_reader_statutes);
}

template <>
std::string to_string(step::video::ff::ReadingMode mode)
{
    return find_by_type(mode, g_reading_modes);
}

template <>
void from_string(step::video::ff::ReaderState& state, const std::string& str)
{
    find_by_str(str, state, g_reader_statutes);
}

template <>
void from_string(step::video::ff::ReadingMode& mode, const std::string& str)
{
    find_by_str(str, mode, g_reading_modes);
}

}  // namespace step::utils