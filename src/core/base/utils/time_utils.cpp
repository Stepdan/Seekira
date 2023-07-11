#include "time_utils.hpp"
#include "string_utils.hpp"

#include <core/log/log.hpp>

namespace step::utils {

#define TIME_TO_STRING(TIME_TYPE)                                                                                      \
    template <>                                                                                                        \
    std::string to_string(TIME_TYPE time)                                                                              \
    {                                                                                                                  \
        return fmt::to_string(time);                                                                                   \
    }

#define TIME_FROM_STRING(TIME_TYPE)                                                                                    \
    template <>                                                                                                        \
    void from_string(TIME_TYPE& time, const std::string& str)                                                          \
    {                                                                                                                  \
        time = TIME_TYPE(std::stoi(str));                                                                              \
    }

STEP_ALL_TIME_TYPES_MACRO(TIME_TO_STRING)
STEP_ALL_TIME_TYPES_MACRO(TIME_FROM_STRING)

}  // namespace step::utils