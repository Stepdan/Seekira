#pragma once

#include <string_view>

namespace detail {

using namespace std::literals::string_view_literals;
constexpr auto kTRACE = "TRACE"sv;
constexpr auto kDEBUG = "DEBUG"sv;
constexpr auto kINFO = "INFO"sv;
constexpr auto kWARNING = "WARNING"sv;
constexpr auto kERROR = "ERROR"sv;
constexpr auto kCRITICAL = "CRITICAL"sv;
constexpr auto kOFF = "OFF"sv;
}  // namespace detail

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define SPDLOG_LEVEL_NAMES                                                                         \
    {                                                                                              \
        detail::kTRACE, detail::kDEBUG, detail::kINFO, detail::kWARNING, detail::kERROR,           \
            detail::kCRITICAL, detail::kOFF                                                        \
    }

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

// For printing classes with overriden stream operator '<<' (like std::thread::thread_id)
#include <spdlog/fmt/ostr.h>

namespace step::log {
using LOG_LEVEL = ::spdlog::level::level_enum;

#define L_TRACE ::spdlog::level::trace
#define L_DEBUG ::spdlog::level::debug
#define L_INFO ::spdlog::level::info
#define L_WARN ::spdlog::level::warn
#define L_ERROR ::spdlog::level::err
#define L_CRITICAL ::spdlog::level::critical
#define L_OFF ::spdlog::level::off
}

#ifndef STEPKIT_MODULE_NAME
#warning STEPKIT_MODULE_NAME undefined, will be skipped
#define STEPKIT_MODULE_NAME ""
#endif
