#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace step::threading {

struct ThreadConditionalParam
{
public:
    ThreadConditionalParam(bool init_value) : param(init_value) {}

    template <typename Predicate>
    void wait(Predicate _p)
    {
        std::unique_lock lock(mutex);
        cnd.wait(lock, _p);
    }

    void set_value(bool value) { param.store(value); }
    bool get_value() { return param; }

    void notify_one() { cnd.notify_one(); }
    void notify_all() { cnd.notify_all(); }

private:
    std::atomic_bool param;
    std::mutex mutex;
    std::condition_variable cnd;
};

}  // namespace step::threading