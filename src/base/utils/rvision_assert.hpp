#pragma once

#include <core/utils/throw_utils.hpp>

#include <fmt/format.h>

#include <sstream>

namespace step::utils::detail {
[[noreturn]] inline void handle_failed_assertion(const char* expr_msg)
{
    std::stringstream what;
    what << "Assertion `" << expr_msg << "` failed.";
    step::utils::throw_with_trace(std::runtime_error(what.str()));
}

template <typename... Args>
[[noreturn]] inline void handle_failed_assertion(const char* expr_msg, const fmt::format_string<Args...>& format,
                                                 Args&&... args)
{
    std::stringstream what;
    what << "Assertion `" << expr_msg << "` failed.";
    if (std::string extra = fmt::format(format, std::forward<Args>(args)...); !extra.empty())
    {
        what << " Explanation: " << extra;
    }

    step::utils::throw_with_trace(std::runtime_error(what.str()));
}
}  // namespace step::utils::detail

/*! @brief Asserts (in the form of an exception being thrown) if an expression
           @a expr evaluates to @c false.

    Asserts by throwing an exception. Provides stack trace information.

    @note You could optionally pass fmt::format()-styled thorough description
    after the expression to assert on. The message would be added to the error
    in case of the assertion.
*/
#define rv_assert(expr, ...)                                                                                           \
    if (!(expr))                                                                                                       \
    step::utils::detail::handle_failed_assertion(#expr, ##__VA_ARGS__)
