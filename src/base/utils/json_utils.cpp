#include "json_utils.hpp"
#include "string_utils.hpp"
#include "throw_utils.hpp"

#include <Poco/JSON/Parser.h>
#include <Poco/Net/HTTPStreamFactory.h>
#include <Poco/URIStreamOpener.h>

#include <fmt/format.h>

#include <log/log.hpp>

#include <fstream>

namespace step::json {

ObjectPtrJSON make_object_json_ptr() { return Poco::makeShared<ObjectJSON>(); }
ArrayPtrJSON make_array_json_ptr() { return Poco::makeShared<ArrayJSON>(); }

ObjectPtrJSON opt_object(const ObjectPtrJSON& container, const std::string& key)
{
    rv_assert(container);
    return container->getObject(key);
}

ArrayPtrJSON opt_array(const ObjectPtrJSON& container, const std::string& key)
{
    rv_assert(container);
    return container->getArray(key);
}

ObjectPtrJSON get_object(const ObjectPtrJSON& container, const std::string& key)
{
    rv_assert(container);
    auto object = opt_object(container, key);
    rv_assert(object, "Container passed doesn't have an object for key '{}'", key);
    return object;
}

ArrayPtrJSON get_array(const ObjectPtrJSON& container, const std::string& key)
{
    rv_assert(container);
    ArrayPtrJSON array;
    array = opt_array(container, key);
    rv_assert(array, "Container passed doesn't have an array for key '{}'", key);
    return array;
}

}  // namespace step::json

namespace step::utils {

template <>
std::string to_string(const ObjectJSON& json)
{
    std::stringstream ss;
    json.stringify(ss);
    return ss.str();
}

template <>
std::string to_string(ObjectJSON json)
{
    return step::utils::to_string<const ObjectJSON&>(json);
}

template <>
std::string to_string(const ObjectPtrJSON& json)
{
    return step::utils::to_string<const ObjectJSON&>(*json);
}

template <>
void from_string(ObjectJSON& obj_json, const std::string& str)
{
    try
    {
        Poco::JSON::Parser parser;
        Poco::Dynamic::Var result = parser.parse(str);
        const auto obj_ptr_json = result.extract<Poco::JSON::Object::Ptr>();
        obj_json = *obj_ptr_json;
    }
    catch (const Poco::Exception& ex)
    {
        utils::throw_runtime_with_log(
            fmt::format("Parsing string {} as json Poco::Exception: ({}) {}", str, ex.className(), ex.displayText()));
    }
    catch (std::exception& ex)
    {
        utils::throw_runtime_with_log(fmt::format("Parsing string {} as json std::exception: {}", str, ex.what()));
    }
    catch (...)
    {
        utils::throw_runtime_with_log(fmt::format("Parsing string {} as json caught unknown exception", str));
    }
}

template <>
void from_string(ObjectPtrJSON& obj_json, const std::string& str)
{
    try
    {
        Poco::JSON::Parser parser;
        Poco::Dynamic::Var result = parser.parse(str);
        obj_json = result.extract<Poco::JSON::Object::Ptr>();
    }
    catch (const Poco::Exception& ex)
    {
        utils::throw_runtime_with_log(
            fmt::format("Parsing string {} as json Poco::Exception: ({}) {}", str, ex.className(), ex.displayText()));
    }
    catch (std::exception& ex)
    {
        utils::throw_runtime_with_log(fmt::format("Parsing string {} as json std::exception: {}", str, ex.what()));
    }
    catch (...)
    {
        utils::throw_runtime_with_log(fmt::format("Parsing string {} as json caught unknown exception", str));
    }
}

}  // namespace step::utils

namespace step::json::utils {

namespace {

std::unique_ptr<std::istream> open_uri_as_stream(const std::string& uri)
{
    try
    {
        return std::unique_ptr<std::istream>(Poco::URIStreamOpener::defaultOpener().open(uri));
    }
    catch (const Poco::Exception& ex)
    {
        step::utils::throw_runtime_with_log(
            fmt::format("Open URI {} Poco::Exception: ({}) {}", uri, ex.className(), ex.displayText()));
    }
    catch (std::exception& ex)
    {
        step::utils::throw_runtime_with_log(fmt::format("Open URI {} std::exception: {}", uri, ex.what()));
    }
    catch (...)
    {
        step::utils::throw_runtime_with_log(fmt::format("Open URI {} caught unknown exception", uri));
    }

    return nullptr;
}

std::optional<std::string> read_file(const std::string& path)
{
    auto stream = open_uri_as_stream(path);
    if (!stream)
        return std::nullopt;
    return std::string{std::istreambuf_iterator<char>(*stream), std::istreambuf_iterator<char>()};
}

std::ostream& operator<<(std::ostream& os, const ObjectPtrJSON& j)
{
    static constexpr uint INDENT = 2;
    j->stringify(os, INDENT);
    return os;
}

}  // namespace

bool is_json(const std::string& body)
{
    return (body.front() == '{' && body.back() == '}') || (body.front() == '[' && body.back() == ']');
}

ObjectPtrJSON from_file(const std::string& path)
{
    const auto content = read_file(path);

    if (!content.has_value())
        return nullptr;

    ObjectPtrJSON json;
    step::utils::from_string(json, content.value());
    return json;
}

ObjectPtrJSON config_from_file(const std::string& config_uri)
{
    ObjectPtrJSON config_data;

    auto content = from_file(config_uri);
    if (!content)
    {
        STEP_LOG(L_ERROR, "Can't load config json from {}", config_uri);
        return nullptr;
    }

    if (auto value = json::opt_object(content, "value"))
        return value;

    return content;
}

void save_to_file(const ObjectPtrJSON& config, const std::string& path)
{
    std::fstream stream(path);
    if (stream.is_open())
        stream << config;
    else
        step::utils::throw_runtime_with_log(fmt::format("Failed to open file: {}", path));
}

}  // namespace step::json::utils
