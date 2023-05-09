#pragma once

#include <core/base/utils/type_utils.hpp>

#include <mutex>
#include <type_traits>

namespace step::threading {
namespace detail {

// a type trait that returns a pointer type
template <typename T, typename = void>
struct Ptr
{
    using type = T*;
    static type get(T& x) noexcept { return std::addressof(x); }
};

template <utils::has_arrow_operator T>
struct Ptr<T>
{
    using type = T&;
    static type get(T& x) noexcept { return x; }
};
}  // namespace detail

/*! @brief Provides an always-under-mutex data access.

    This simple adapter allows one to ensure that the stored data is always
    accompanied by the mutex lock by providing a handle through which that data
    is accessed.
*/
template <typename T, typename Mutex = std::mutex>
class ThreadGuard
{
    T m_data{};
    Mutex m_mutex{};

    using ptr_helper = detail::Ptr<T>;

public:
    /*! @brief The catch-all ctor that allows to create data from @a args.
        @note @a args could be empty.
    */
    template <typename... Args>
    ThreadGuard(Args&&... args) : m_data(std::forward<Args>(args)...)
    {
    }

    class AccessHandle;
    /*! @brief Returns a new access handle, which instantly locks an internal
        mutex upon creation.
    */
    AccessHandle lock() noexcept { return AccessHandle(m_data, m_mutex); }

    /*! @brief Returns a new access handle similarly to lock().

        A convenience method that allows arrow operator chaining. Calling
        operator->() could be more useful in scenarios when a single under-mutex
        function call on data is required:
        ```
        // via lock():
        {
            auto handle = guard.lock(); // lock the mutex, acquire a handle
            handle->do_smth_on_data(); // call the operation on data
        }

        // via operator->():
        guard->do_smth_on_data(); // call the operation on data
        ```
    */
    AccessHandle operator->() noexcept { return lock(); }

    /*! @brief Gives the user an ability to read/modify the data.

        The handle provides access to the underlying data. At the same time,
        this handle acts as a lock guard: as long as it is in scope, the
        internal mutex is locked. The same mutex is shared across all instances
        of this class.
    */
    class [[nodiscard]] AccessHandle
    {
    private:
        T& m_data;
        Mutex& m_mutex;

    public:
        AccessHandle(T& data, Mutex& mutex) : m_data(data), m_mutex(mutex) { m_mutex.lock(); }
        ~AccessHandle() { m_mutex.unlock(); }

        using pointer = typename ptr_helper::type;
        pointer operator->() noexcept { return ptr_helper::get(m_data); }
        const pointer operator->() const noexcept { return ptr_helper::get(m_data); }

        T& operator*() noexcept { return get(); }
        const T& operator*() const noexcept { return get(); }

        /*! @brief Returns the underlying data by non-const reference.
        */
        T& get() noexcept { return m_data; }

        /*! @brief Returns the underlying data by const reference.
        */
        const T& get() const noexcept { return m_data; }
    };
};
}  // namespace step::threading
