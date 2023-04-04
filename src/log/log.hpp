#pragma once

#include "log_settings.hpp"

#include <thread>
#include <string_view>
#include <fmt/format.h>

namespace step::log {

class Logger
{
public:
    static Logger& instance();

    void set_settings(LoggingSettings&&);
    void reset();
    void set_log_level(LOG_LEVEL);
    LOG_LEVEL get_log_level() const;
    void dump_backtrace();

    template <typename... Args>
    void log(LOG_LEVEL level, const fmt::format_string<Args...>& str, Args&&... args)
    {
        if (logger_)
            logger_->log(level, str, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void log_to_default(LOG_LEVEL level, const fmt::format_string<Args...>& str, Args&&... args)
    {
        spdlog::log(level, str, std::forward<Args>(args)...);
    }

private:
    Logger();
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

private:
    std::shared_ptr<spdlog::logger> logger_;
};

}  // namespace step::log

// {:>16.16s}; - STEPKIT_MODULE_NAME
#define STEP_LOG(level, format, ...)                                                                                \
    step::log::Logger::instance().log(level, "{:>16.16s}; thread {}; msg=" format " ({}:{}, {})",                   \
                                         STEPKIT_MODULE_NAME, std::this_thread::get_id(), ##__VA_ARGS__, __FILE__,     \
                                         __LINE__, __FUNCTION__)

#define STEP_LOG_TO_DEFAULT_NO_SOURCE(level, format, ...)                                                           \
    step::log::Logger::instance().log_to_default(level, "{:>16.16s}; thread {}; msg=" format, STEPKIT_MODULE_NAME,  \
                                                    std::this_thread::get_id(), ##__VA_ARGS__)

#define STEP_LOG_TO_DEFAULT(level, format, ...)                                                                     \
    step::log::Logger::instance().log_to_default(level, "{:>16.16s}; thread {}; msg=" format " ({}:{}, {})",        \
                                                    STEPKIT_MODULE_NAME, std::this_thread::get_id(), ##__VA_ARGS__,    \
                                                    __FILE__, __LINE__, __FUNCTION__)
