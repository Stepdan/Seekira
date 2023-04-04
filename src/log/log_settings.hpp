#pragma once

#include "log_common.hpp"

namespace step::log {

class LoggingSettings
{
public:
    enum class SyncMode
    {
        Sync,
        Async
    };

public:
    void set_backtrace_size(size_t value) { m_backtrace_size = value; }
    void set_sync_mode(SyncMode value) { m_sync_mode = value; }

    size_t get_backtrace_size() const noexcept { return m_backtrace_size; }
    SyncMode get_sync_mode() const noexcept { return m_sync_mode; }

private:
    size_t m_backtrace_size{1000};
    SyncMode m_sync_mode{SyncMode::Async};
};

}  // namespace step::log
