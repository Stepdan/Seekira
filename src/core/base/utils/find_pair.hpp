#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <string>
#include <type_traits>

namespace step::utils {

template <typename FuncType, typename ArgType>
struct isBoolExecutableWithTwoSameTypeParams
{
private:
    static void detect(...);
    template <typename U, typename V>
    static decltype(std::declval<U>()(std::declval<V>(), std::declval<V>())) detect(const U&, const V&);

public:
    static constexpr bool value =
        std::is_same<bool, decltype(detect(std::declval<FuncType>(), std::declval<ArgType>()))>::value;
};

template <typename InputIterator, typename Value = typename std::iterator_traits<InputIterator>::value_type,
          typename KeyEqual = std::equal_to<typename Value::first_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::first_type>::value,
                                InputIterator>
find_pair_iterator_by_first(InputIterator begin, InputIterator end, const typename Value::first_type& key,
                            KeyEqual comparer = KeyEqual{})
{
    return std::find_if(begin, end, std::bind(comparer, key, std::bind(&Value::first, std::placeholders::_1)));
}

template <typename Container, typename Value = typename Container::value_type,
          typename KeyEqual = std::equal_to<typename Value::first_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::first_type>::value,
                                typename Container::const_iterator>
find_pair_iterator_by_first(const Container& container, const typename Value::first_type& key,
                            KeyEqual comparer = KeyEqual{})
{
    return find_pair_iterator_by_first(container.begin(), container.end(), key, comparer);
}

template <typename Value, size_t ArraySize, typename KeyEqual = std::equal_to<typename Value::first_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::first_type>::value,
                                Value*>
find_pair_iterator_by_first(Value (&array)[ArraySize], const typename Value::first_type& key,
                            KeyEqual comparer = KeyEqual{})
{
    return find_pair_iterator_by_first(std::cbegin(array), std::cend(array), key, comparer);
}

template <typename InputIterator, typename Value = typename std::iterator_traits<InputIterator>::value_type,
          typename KeyEqual = std::equal_to<typename Value::first_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::first_type>::value,
                                const typename Value::second_type&>
find_second(InputIterator begin, InputIterator end, const typename Value::first_type& key,
            KeyEqual comparer = KeyEqual{})
{
    const auto it = find_pair_iterator_by_first(begin, end, key, comparer);
    assert(it != end);
    return it->second;
}

template <typename Container, typename Value = typename Container::value_type,
          typename KeyEqual = std::equal_to<typename Value::first_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::first_type>::value,
                                const typename Value::second_type&>
find_second(const Container& container, const typename Value::first_type& key, KeyEqual comparer = KeyEqual{})
{
    return find_second(container.begin(), container.end(), key, comparer);
}

template <typename Value, size_t ArraySize, typename KeyEqual = std::equal_to<typename Value::first_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::first_type>::value,
                                const typename Value::second_type&>
find_second(Value (&array)[ArraySize], const typename Value::first_type& key, KeyEqual comparer = KeyEqual{})
{
    return find_second(std::cbegin(array), std::cend(array), key, comparer);
}

template <typename InputIterator, typename Value = typename std::iterator_traits<InputIterator>::value_type,
          typename KeyEqual = std::equal_to<typename Value::first_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::first_type>::value,
                                const typename Value::second_type&>
find_second(InputIterator begin, InputIterator end, const typename Value::first_type& key,
            const typename Value::second_type& defaultValue, KeyEqual comparer = KeyEqual{})
{
    const auto it = find_pair_iterator_by_first(begin, end, key, comparer);
    return it != end ? it->second : defaultValue;
}

template <typename Container, typename Value = typename Container::value_type,
          typename KeyEqual = std::equal_to<typename Value::first_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::first_type>::value,
                                const typename Value::second_type&>
find_second(const Container& container, const typename Value::first_type& key,
            const typename Value::second_type& defaultValue, KeyEqual comparer = KeyEqual{})
{
    return find_second(container.begin(), container.end(), key, defaultValue, comparer);
}

template <typename Value, size_t ArraySize, typename KeyEqual = std::equal_to<typename Value::first_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::first_type>::value,
                                const typename Value::second_type&>
find_second(Value (&array)[ArraySize], const typename Value::first_type& key,
            const typename Value::second_type& defaultValue, KeyEqual comparer = KeyEqual{})
{
    return find_second(std::cbegin(array), std::cend(array), key, defaultValue, comparer);
}

template <typename InputIterator, typename Value = typename std::iterator_traits<InputIterator>::value_type,
          typename KeyEqual = std::equal_to<typename Value::second_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::second_type>::value,
                                InputIterator>
find_pair_iterator_by_second(InputIterator begin, InputIterator end, const typename Value::second_type& key,
                             KeyEqual comparer = KeyEqual{})
{
    return std::find_if(begin, end, std::bind(comparer, key, std::bind(&Value::second, std::placeholders::_1)));
}

template <typename Container, typename Value = typename Container::value_type,
          typename KeyEqual = std::equal_to<typename Value::second_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::second_type>::value,
                                typename Container::const_iterator>
find_pair_iterator_by_second(const Container& container, const typename Value::second_type& key,
                             KeyEqual comparer = KeyEqual{})
{
    return find_pair_iterator_by_second(container.begin(), container.end(), key, comparer);
}

template <typename Value, size_t ArraySize, typename KeyEqual = std::equal_to<typename Value::second_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::second_type>::value,
                                Value*>
find_pair_iterator_by_second(Value (&array)[ArraySize], const typename Value::second_type& key,
                             KeyEqual comparer = KeyEqual{})
{
    return find_pair_iterator_by_second(std::cbegin(array), std::cend(array), key, comparer);
}

template <typename InputIterator, typename Value = typename std::iterator_traits<InputIterator>::value_type,
          typename KeyEqual = std::equal_to<typename Value::second_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::second_type>::value,
                                const typename Value::first_type&>
find_first(InputIterator begin, InputIterator end, const typename Value::second_type& key,
           KeyEqual comparer = KeyEqual{})
{
    const auto it = find_pair_iterator_by_second(begin, end, key, comparer);
    assert(it != end);
    return it->first;
}

template <typename Container, typename Value = typename Container::value_type,
          typename KeyEqual = std::equal_to<typename Value::second_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::second_type>::value,
                                const typename Value::first_type&>
find_first(const Container& container, const typename Value::second_type& key, KeyEqual comparer = KeyEqual{})
{
    return find_first(container.begin(), container.end(), key, comparer);
}

template <typename Value, size_t ArraySize, typename KeyEqual = std::equal_to<typename Value::second_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::second_type>::value,
                                const typename Value::first_type&>
find_first(Value (&array)[ArraySize], const typename Value::second_type& key, KeyEqual comparer = KeyEqual{})
{
    return find_first(std::cbegin(array), std::cend(array), key, comparer);
}

template <typename InputIterator, typename Value = typename std::iterator_traits<InputIterator>::value_type,
          typename KeyEqual = std::equal_to<typename Value::second_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::second_type>::value,
                                const typename Value::first_type&>
find_first(InputIterator begin, InputIterator end, const typename Value::second_type& key,
           const typename Value::first_type& defaultValue, KeyEqual comparer = KeyEqual{})
{
    const auto it = find_pair_iterator_by_second(begin, end, key, comparer);
    return it != end ? it->first : defaultValue;
}

template <typename Container, typename Value = typename Container::value_type,
          typename KeyEqual = std::equal_to<typename Value::second_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::second_type>::value,
                                const typename Value::first_type&>
find_first(const Container& container, const typename Value::second_type& key,
           const typename Value::first_type& defaultValue, KeyEqual comparer = KeyEqual{})
{
    return find_first(container.begin(), container.end(), key, defaultValue, comparer);
}

template <typename Value, size_t ArraySize, typename KeyEqual = std::equal_to<typename Value::second_type>>
const typename std::enable_if_t<isBoolExecutableWithTwoSameTypeParams<KeyEqual, typename Value::second_type>::value,
                                const typename Value::first_type&>
find_first(Value (&array)[ArraySize], const typename Value::second_type& key,
           const typename Value::first_type& defaultValue, KeyEqual comparer = KeyEqual{})
{
    return find_first(std::cbegin(array), std::cend(array), key, defaultValue, comparer);
}

template <typename T>
struct RefWrapComparer
{
    inline bool operator()(const T& lh, const std::reference_wrapper<const T>& rh) const { return lh == rh.get(); }

    inline bool operator()(const std::reference_wrapper<const T>& lh, const T& rh) const { return lh.get() == rh; }

    inline bool operator()(const std::reference_wrapper<const T>& lh, const std::reference_wrapper<const T>& rh) const
    {
        return lh.get() == rh.get();
    }
};

using RefWrapComparerString = RefWrapComparer<std::string>;

template <typename T, typename C>
std::string find_by_type(const T& type, const C& container)
{
    if (auto it = step::utils::find_pair_iterator_by_first(container, type); it != std::cend(container))
        return std::string(it->second);
    else
        return std::string();
}

template <typename T, typename C>
void find_by_str(const std::string& str, T& type, const C& container)
{
    if (auto it = step::utils::find_pair_iterator_by_second(container, str); it != std::cend(container))
        type = it->first;
    else
        type = std::remove_reference_t<decltype(type)>::Undefined;
}

template <typename T, typename C>
std::vector<std::string> find_all_by_type(const T& type, const C& container)
{
    std::vector<std::string> result;
    std::for_each(std::cbegin(container), std::cend(container), [&result, &type](const auto& item) {
        if (item.first == type)
            result.push_back(std::string(item.second));
    });

    return result;
}

}  // namespace step::utils
