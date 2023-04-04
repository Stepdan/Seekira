#pragma once

#include <condition_variable>
#include <mutex>

namespace step::utils {

template <typename T>
struct ThreadConditionalParam
{
public:
    ThreadConditionalParam(const T& init_value) : param(init_value){};

    template <typename Predicate>
    void wait(Predicate _p)
    {
        std::unique_lock lock(mutex);
        cnd.wait(lock, _p);
    }

    void set_value(const T& value)
    {
        std::unique_lock lock(mutex);
        param = value;
    }

    T param;
    std::mutex mutex;
    std::condition_variable cnd;
};

}  // namespace step::utils