#include "base/utils/exception/throw_utils.hpp"

#include <log/log.hpp>

namespace step::utils {

const StackTrace* backtrace_exception(const std::exception& e) { return boost::get_error_info<TraceInfo>(e); }

void throw_runtime_with_log(const std::string& msg)
{
    STEP_LOG(L_ERROR, "{}", msg);
    throw_with_trace(std::runtime_error(msg));
}

}  // namespace step::utils
