#include "log.hpp"

#include <spdlog/common.h>
#include <spdlog/cfg/env.h>

namespace {

constexpr const char* LOGGER_NAME = "stepkit_logger";

// Message example
// 2019-07-02 15:56:12.333333;   INFO;              D2R; msg=Diagnostic message
constexpr const char* LOG_PATTERN = "%Y-%m-%d %H:%M:%S.%f; %^%8l%$; %v";

spdlog::sink_ptr CreateStderrSink(step::log::LoggingSettings::SyncMode syncMode, step::log::LOG_LEVEL level)
{
    spdlog::sink_ptr sink;
    if (syncMode == step::log::LoggingSettings::SyncMode::Sync)
        sink = std::make_shared<spdlog::sinks::stderr_color_sink_st>();
    else
        sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    sink->set_level(level);

    return sink;
}

}  // namespace

namespace step::log {

Logger::Logger()
{
    set_settings(LoggingSettings());

    // Set default logger to stderr
    // It will be used in LogToDefault
    auto defaultLogger = spdlog::stderr_color_mt("default_logger");
    defaultLogger->set_pattern(LOG_PATTERN);
    spdlog::set_default_logger(defaultLogger);
}

Logger& Logger::instance()
{
    static Logger obj;
    return obj;
}

void Logger::set_settings(LoggingSettings&& settings)
{
    reset();

    // Create stderr sink with level L_TRACE to grab all messages in backtrace
    logger_ = std::make_shared<spdlog::logger>(LOGGER_NAME, CreateStderrSink(settings.get_sync_mode(), L_TRACE));
    if (settings.get_backtrace_size() != 0)
        logger_->enable_backtrace(settings.get_backtrace_size());

    logger_->set_pattern(LOG_PATTERN);

    // register the actual logger through the spdlog system. this allows, in
    // particular, to control log level, etc. via environment variables

    // Note: we only allow Logger::instance() singleton, so this static-lifetime
    // registration is safe
    spdlog::register_logger(logger_);
    spdlog::cfg::load_env_levels();
}

void Logger::reset()
{
    if (!logger_)
        return;

    spdlog::shutdown();
    logger_.reset();
}

void Logger::set_log_level(LOG_LEVEL level)
{
    if (logger_)
        logger_->set_level(level);
}

LOG_LEVEL Logger::get_log_level() const { return logger_ ? logger_->level() : L_OFF; }

void Logger::dump_backtrace()
{
    if (logger_)
        logger_->dump_backtrace();
}

}  // namespace step::log
