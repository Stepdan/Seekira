#pragma once

#include <QMessageLogContext>
#include <QString>

#include <fmt/format.h>

namespace step::gui::utils {

void qt_log_handler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

}

template <>
struct fmt::formatter<QString> : fmt::formatter<string_view>
{
    auto format(const QString& s, fmt::format_context& ctx)
    {
        return fmt::formatter<string_view>::format({s.toUtf8(), static_cast<size_t>(s.size())}, ctx);
    }
};