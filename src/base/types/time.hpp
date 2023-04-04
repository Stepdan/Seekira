#pragma once

#include <fmt/format.h>

#include <chrono>
#include <limits>

#define RVISION_ALL_TIME_TYPES_MACRO(MACRO, ...)                                                                       \
    MACRO(step::Nanoseconds, ##__VA_ARGS__)                                                                         \
    MACRO(step::Microseconds, ##__VA_ARGS__)                                                                        \
    MACRO(step::Milliseconds, ##__VA_ARGS__)                                                                        \
    MACRO(step::Seconds, ##__VA_ARGS__)                                                                             \
    MACRO(step::Minutes, ##__VA_ARGS__)                                                                             \
    MACRO(step::Hours, ##__VA_ARGS__)

namespace rvision {

using Nanoseconds = std::chrono::nanoseconds;
using Microseconds = std::chrono::microseconds;
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;
using Minutes = std::chrono::minutes;
using Hours = std::chrono::hours;

using TimeType = std::chrono::microseconds::rep;
// TODO: these has to be templated, otherwise we cannot compare them to arbitrary time!
constexpr TimeType INVALID_TIME = std::numeric_limits<TimeType>::min();
constexpr TimeType INFINITE_TIME = std::numeric_limits<TimeType>::max();

// Default timestamp in ARRIVAL is microseconds.
using Timestamp = Microseconds;
constexpr Timestamp INVALID_TIMESTAMP = Timestamp(INVALID_TIME);

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

// TODO: delete these 3, they do practically nothing
bool is_invalid_timestamp(const Timestamp& ts);
bool is_infinite_timestamp(const Timestamp& ts);
Timestamp get_current_timestamp();

}  // namespace rvision

#define TIME_FORMATTER(TIME_TYPE)                                                                                      \
    template <>                                                                                                        \
    struct fmt::formatter<TIME_TYPE> : formatter<string_view>                                                          \
    {                                                                                                                  \
        template <typename FormatContext>                                                                              \
        auto format(const TIME_TYPE& time, FormatContext& ctx)                                                         \
        {                                                                                                              \
            return format_to(ctx.out(), "{}", time.count());                                                           \
        }                                                                                                              \
    };

RVISION_ALL_TIME_TYPES_MACRO(TIME_FORMATTER)