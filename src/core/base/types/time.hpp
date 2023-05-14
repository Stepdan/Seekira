#pragma once

#include <fmt/format.h>

#include <chrono>
#include <limits>

#define STEP_ALL_TIME_TYPES_MACRO(MACRO)                                                                               \
    MACRO(step::Nanoseconds)                                                                                           \
    MACRO(step::Microseconds)                                                                                          \
    MACRO(step::Milliseconds)                                                                                          \
    MACRO(step::Seconds)                                                                                               \
    MACRO(step::Minutes)                                                                                               \
    MACRO(step::Hours)

namespace step {

using Nanoseconds = std::chrono::nanoseconds;
using Microseconds = std::chrono::microseconds;
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;
using Minutes = std::chrono::minutes;
using Hours = std::chrono::hours;

using TimeType = std::chrono::microseconds::rep;

constexpr TimeType INVALID_TIME = std::numeric_limits<TimeType>::min();
constexpr TimeType INFINITE_TIME = std::numeric_limits<TimeType>::max();

template <typename TTime>
bool is_invalid_time(const TTime& time)
{
    // TODO: do *NOT* compare counts. compare objects!
    return time.count() == INVALID_TIME;
}

template <typename TTime>
bool is_infinite_time(const TTime& time)
{
    return time.count() == INFINITE_TIME;
}

template <typename TTime>
TTime get_current_time()
{
    // TODO: use steady_clock!
    return std::chrono::duration_cast<TTime>(std::chrono::system_clock::now().time_since_epoch());
}

// StepKit's default type of timestamp is microseconds
using Timestamp = Microseconds;

bool is_invalid_timestamp(const Timestamp& ts);
bool is_infinite_timestamp(const Timestamp& ts);
Timestamp get_current_timestamp();

}  // namespace step

#define TIME_FORMATTER(TIME_TYPE)                                                                                      \
    template <>                                                                                                        \
    struct fmt::formatter<TIME_TYPE> : fmt::formatter<string_view>                                                     \
    {                                                                                                                  \
        template <typename FormatContext>                                                                              \
        auto format(const TIME_TYPE& time, FormatContext& ctx)                                                         \
        {                                                                                                              \
            return fmt::format_to(ctx.out(), "{}", time.count());                                                      \
        }                                                                                                              \
    };

STEP_ALL_TIME_TYPES_MACRO(TIME_FORMATTER)
