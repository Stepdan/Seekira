#include "log_utils.hpp"

#include <log/log.hpp>

namespace step::utils {

void log_info_msg(const std::string& str) { STEP_LOG(L_INFO, "{}", str); }

}  // namespace step::utils
