#include "log_handler.hpp"

#include <core/log/log.hpp>
#include <core/exception/assert.hpp>

#include <fmt/color.h>

namespace step::gui::utils {

void qt_log_handler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    spdlog::level::level_enum level{L_CRITICAL};
    std::string prefix{};

    switch (type)
    {
        case QtDebugMsg:
            level = L_DEBUG;
            prefix = fmt::format(fmt::fg(fmt::terminal_color::blue), "[QT_DEBUG]");
            break;
        case QtInfoMsg:
            level = L_INFO;
            prefix = fmt::format(fmt::fg(fmt::terminal_color::green), "[QT_INFO]");
            break;
        case QtWarningMsg:
            level = L_WARN;
            prefix = fmt::format(fmt::fg(fmt::terminal_color::yellow), "[QT_WARNING]");
            break;
        case QtCriticalMsg:
            level = L_CRITICAL;
            prefix = fmt::format(fmt::fg(fmt::terminal_color::red), "[QT_CRITICAL]");
            break;
        case QtFatalMsg:
            level = L_CRITICAL;
            prefix = fmt::format(fmt::fg(fmt::terminal_color::bright_red), "[QT_FATAL]");
    }

    auto value_or = [](const char* value, const char* or_value) { return value != nullptr ? value : or_value; };

    STEP_LOG(level, "{}: {} ({}:{}, {})", prefix, msg, value_or(context.file, "<file>"), context.line,
             value_or(context.function, "<function>"));

    if (type == QtFatalMsg)
    {
        STEP_THROW_RUNTIME("Catch fatal message from Qt");
    }
}

}  // namespace step::gui::utils