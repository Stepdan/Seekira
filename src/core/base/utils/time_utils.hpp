#pragma once

#include <core/log/log.hpp>

#include <core/base/types/time.hpp>

#include <optional>
#include <string>

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

template <typename TTime>
class ExecutionTimer
{
public:
    ExecutionTimer(const std::string& tag) : m_init_time(get_current_time<TTime>()), m_tag(tag) {}
    ~ExecutionTimer()
    {
        auto time = get_current_time<TTime>() - m_init_time;
        STEP_LOG(L_INFO, "Execution time ({}): {}", m_tag, time.count());
    }

private:
    TTime m_init_time;
    std::string m_tag;
};

}  // namespace step::utils