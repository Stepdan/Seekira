#include "log.hpp"

#include <spdlog/common.h>
#include <spdlog/cfg/env.h>

namespace {

constexpr const char* m_loggerNAME = "stepkit_logger";

constexpr const char* LOG_PATTERN = "%Y-%m-%d %H:%M:%S.%f; %^%8l%$; thread %t; %v";

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
    m_logger = std::make_shared<spdlog::logger>(m_loggerNAME, CreateStderrSink(settings.get_sync_policy(), L_TRACE));
    if (settings.get_backtrace_size() != 0)
        m_logger->enable_backtrace(settings.get_backtrace_size());

    m_logger->set_pattern(LOG_PATTERN);

    // register the actual logger through the spdlog system. this allows, in
    // particular, to control log level, etc. via environment variables

    // Note: we only allow Logger::instance() singleton, so this static-lifetime
    // registration is safe
    spdlog::register_logger(m_logger);
    spdlog::cfg::load_env_levels();
}

void Logger::reset()
{
    if (!m_logger)
        return;

    spdlog::shutdown();
    m_logger.reset();
}

void Logger::set_log_level(LOG_LEVEL level)
{
    if (m_logger)
        m_logger->set_level(level);
}

LOG_LEVEL Logger::get_log_level() const { return m_logger ? m_logger->level() : L_OFF; }

void Logger::dump_backtrace()
{
    if (m_logger)
        m_logger->dump_backtrace();
}

}  // namespace step::log
