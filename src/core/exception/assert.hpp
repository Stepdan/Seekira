#pragma once

#include "throw_utils.hpp"

#include <fmt/format.h>

#include <sstream>

namespace step::exception::detail {

[[noreturn]] inline void handle_failed_assertion(const char* expr_msg)
{
    std::stringstream what;
    what << "Assertion `" << expr_msg << "` failed.";
    step::exception::throw_runtime_with_log(what.str());
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

    step::exception::throw_runtime_with_log(what.str());
}

template <typename... Args>
[[noreturn]] inline void handle_runtime_exception(const fmt::format_string<Args...>& format, Args&&... args)
{
    std::stringstream what;
    what << fmt::format(format, std::forward<Args>(args)...);
    step::exception::throw_runtime_with_log(what.str());
}

}  // namespace step::exception::detail

/*! @brief Asserts (in the form of an exception being thrown) if an expression
           @a expr evaluates to @c false.

    Asserts by throwing an exception. Provides stack trace information.

    @note You could optionally pass fmt::format()-styled thorough description
    after the expression to assert on. The message would be added to the error
    in case of the assertion.
*/
#define STEP_ASSERT(expr, ...)                                                                                         \
    if (!(expr))                                                                                                       \
    step::exception::detail::handle_failed_assertion(#expr, ##__VA_ARGS__)

#define STEP_UNDEFINED(...) step::exception::detail::handle_failed_assertion(##__VA_ARGS__)

#define STEP_THROW_RUNTIME(fmt_msg, ...) step::exception::detail::handle_runtime_exception(fmt_msg, ##__VA_ARGS__)
