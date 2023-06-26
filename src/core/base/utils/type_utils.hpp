#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <limits>
#include <vector>
#include <type_traits>

namespace step::utils {

/* clang-format off */
template <std::floating_point T>
bool compare(T x, T y)
{
    // see https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
    constexpr int UNITS_IN_LAST_PLACE = 7;
    return false
        || std::fabs(x - y) <= std::numeric_limits<T>::epsilon() * std::fabs(x + y) * UNITS_IN_LAST_PLACE
        || std::fabs(x - y) < std::numeric_limits<T>::min();
}

// greater or equal
template <std::floating_point T>
bool compare_ge(T x, T y)
{
    return x > y || compare(x, y);
}

// less or equal
template <std::floating_point T>
bool compare_le(T x, T y)
{
    return x < y || compare(x, y);
}
/* clang-format on */

template <std::floating_point T>
bool compare(const std::vector<T>& lhs, const std::vector<T>& rhs)
{
    if (rhs.size() != lhs.size())
        return false;

    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](T a, T b) { return compare<T>(a, b); });
}

template <typename T, typename = void>
struct is_iterable : std::false_type
{
};

template <typename T>
struct is_iterable<T, std::void_t<typename T::iterator>> : std::true_type
{
};

template <typename T, typename = void>
struct has_arrow_operator_trait : std::false_type
{
};

template <typename T>
struct has_arrow_operator_trait<T, std::void_t<decltype(std::declval<T>().operator->())>> : std::true_type
{
};

template <typename T>
concept has_arrow_operator = has_arrow_operator_trait<T>::value;

// Note: somehow arithmetic concept is not in the std lib
template <typename T>
concept arithmetic = std::is_arithmetic_v<T>;

}  // namespace step::utils
