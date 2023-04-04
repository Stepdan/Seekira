#pragma once

#include "json_types.hpp"

#include <algorithm>
#include <string>
#include <vector>
#include <string_view>

namespace step::utils {

std::vector<std::string> split(std::string s, const std::string& d);

bool starts_with(std::string_view s, std::string_view prefix);

}  // namespace step::utils

namespace step::utils {

template <typename T>
std::string to_string(T);

template <typename T>
std::vector<std::string> to_strings(T);

template <typename T>
void from_string(T&, const std::string&);

template <typename T>
T from_string(std::string_view);

template <typename T>
std::vector<std::string> to_strings(const std::vector<T>& data)
{
    std::vector<std::string> result;

    for (const auto& item : data)
    {
        auto strings = to_strings(item);
        if (!strings.empty())
            result.insert(result.end(), strings.cbegin(), strings.cend());
    }

    return result;
}

template <typename T>
void from_strings(std::vector<T>& result, const std::vector<std::string>& strings)
{
    result.clear();
    result.reserve(strings.size());
    std::transform(strings.cbegin(), strings.cend(), std::back_inserter(result), [](const auto& item) {
        T type;
        from_string(type, item);
        return type;
    });
}

template <typename T>
void from_json_array(std::vector<T>& result, const ArrayPtrJSON& array)
{
    result.reserve(array->size());
    result.resize(array->size());
    for (size_t i = 0; i < array->size(); ++i)
        result[i] = step::utils::from_string<T>(std::next(array->begin(), i)->extract<std::string>());
}

}  // namespace step::utils
