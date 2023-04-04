#pragma once

#include <boost/stacktrace.hpp>
#include <boost/exception/all.hpp>
#include <limits>

namespace step::utils {

using StackTrace = boost::stacktrace::stacktrace;

using TraceInfo = boost::error_info<struct stactrace_tag, StackTrace>;

template <class E>
[[noreturn]] BOOST_FORCEINLINE void throw_with_trace(const E& e,
                                                     size_t stack_limit = std::numeric_limits<size_t>::max()) {
    throw boost::enable_error_info(e) << TraceInfo(StackTrace(0, stack_limit));
}

const StackTrace* backtrace_exception(const std::exception& e);

[[noreturn]] void throw_runtime_with_log(const std::string& msg);

}  // namespace step::utils
