#pragma once

#include <core/base/types/time.hpp>

#include <optional>

namespace step::utils {

template <typename TTime>
class ExpirationTimer
{
public:
    explicit ExpirationTimer(TimeType timeout) : timeout_(timeout), init_time_(get_current_time<TTime>()) {}
    explicit ExpirationTimer(TTime timeout) : timeout_(timeout), init_time_(get_current_time<TTime>()) {}

    bool is_expired() const noexcept { return count() >= timeout_; }

    TTime count() const noexcept { return get_current_time<TTime>() - init_time_; }

    std::optional<TTime> remaining_time() const noexcept
    {
        const auto c = count();
        if (timeout_ < c)
        {
            return std::nullopt;
        }
        return timeout_ - c;
    }

private:
    TTime timeout_;
    TTime init_time_;
};

}  // namespace step::utils