#pragma once

#include "global.hpp"

#include <boost/stacktrace.hpp>
#include <boost/exception/all.hpp>

#include <limits>

namespace step::utils {

using StackTrace = boost::stacktrace::stacktrace;

constexpr size_t STACK_LIMIT_MAX = std::numeric_limits<size_t>::max();

using TraceInfo = boost::error_info<struct stactrace_tag, StackTrace>;

template <class E>
[[noreturn]] BOOST_FORCEINLINE void throw_with_trace(const E& e,
                                                     size_t stack_limit = STACK_LIMIT_MAX) {
    throw boost::enable_error_info(e) << TraceInfo(StackTrace(0, stack_limit));
}

const StackTrace* backtrace_exception(const std::exception& e);

[[noreturn]] void throw_runtime_with_log(const std::string& msg);

}  // namespace step::utils
