#include "string_utils.hpp"

namespace step::utils {

std::vector<std::string> split(std::string s, const std::string& d)
{
    std::vector<std::string> strings;

    size_t start = 0U;
    size_t end = s.find(d);
    while (end != std::string::npos)
    {
        strings.emplace_back(std::move(s.substr(start, end - start)));
        start = end + d.length();
        end = s.find(d, start);
    }
    strings.emplace_back(std::move(s.substr(start, end)));

    return strings;
}

bool starts_with(std::string_view s, std::string_view prefix)
{
    if (prefix.length() > s.length())
    {
        return false;
    }
    s.remove_suffix(s.length() - prefix.length());
    return prefix == s;
}

}  // namespace step::utils
