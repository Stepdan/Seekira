#include "time.hpp"

namespace rvision {

bool is_invalid_timestamp(const Timestamp& ts) { return is_invalid_time<Timestamp>(ts); }

bool is_infinite_timestamp(const Timestamp& ts) { return is_infinite_time<Timestamp>(ts); }

Timestamp get_current_timestamp() { return get_current_time<Timestamp>(); }

}  // namespace rvision