#pragma once

#include "json_types.hpp"

#include <core/utils/throw_utils.hpp>
#include <core/utils/rvision_assert.hpp>

#include <fmt/format.h>

#include <optional>
#include <type_traits>
#include <vector>

namespace step::json::utils {

bool is_json(const std::string& body);

ObjectPtrJSON from_file(const std::string& path);
ObjectPtrJSON config_from_file(const std::string& config_uri);

void save_to_file(const ObjectPtrJSON& config, const std::string& path);

}  // namespace step::json::utils

namespace step::json {

ObjectPtrJSON make_object_json_ptr();
ArrayPtrJSON make_array_json_ptr();

/*! @brief Sets a new value @a value for the key @a key.
*/
template <typename T>
void set(ObjectPtrJSON& container, const std::string& key, const T& value)
{
    rv_assert(container);
    container->set(key, value);
}

/*! @brief Sets a new value contained in an @a optional for the key @a key when
           the optional contains a valid value.
*/
template <typename T>
void set_opt(ObjectPtrJSON& container, const std::string& key, const std::optional<T>& optional)
{
    rv_assert(container);
    if (!optional.has_value())
        return;
    set(container, key, optional.value());
}

template <typename T>
void add(ArrayPtrJSON& container, T&& value)
{
    rv_assert(container);
    container->add(std::forward<T>(value));
}

/*! @brief Returns a value of a user-specified type from @a container with key
           @a key. Throws in case of errors.
*/
template <typename T>
T get(const ObjectPtrJSON& container, const std::string& key) noexcept(false)
{
    rv_assert(container);
    rv_assert(container->has(key), "Key '{}' doesn't exist", key);

    try
    {
        return container->getValue<T>(key);
    }
    catch (const Poco::Exception& e)
    {
        step::utils::throw_runtime_with_log(
            fmt::format("POCO's getValue failed with '{}' for key: {}", e.what(), key));
    }
}

/*! @brief Returns a value of a user-specified type from @a container with key
           @a key. If the value doesn't exist, returns a default value @a def.
*/
template <typename T>
T get(const ObjectPtrJSON& container, const std::string& key, const T& def) noexcept
{
    rv_assert(container);
    return container->optValue(key, def);
}

/*! @brief Returns a value of a user-specified type from @a container with key
           @a key, wrapped in a @c std::optional. If the value doesn't exist,
           returns @c std::nullopt.
*/
template <typename T>
std::optional<T> get_opt(const ObjectPtrJSON& container, const std::string& key)
{
    rv_assert(container);
    try
    {
        return container->getValue<T>(key);
    }
    catch (const Poco::Exception& e)
    {
        return std::nullopt;
    }
}

/*! @brief Returns a valid (non-nullptr) JSON object pointer. Throws in case of
           errors.
*/
ObjectPtrJSON get_object(const ObjectPtrJSON& container, const std::string& key);

/*! @brief Returns a valid (non-nullptr) JSON array pointer. Throws in case of
           errors.
*/
ArrayPtrJSON get_array(const ObjectPtrJSON& container, const std::string& key);

/*! @brief Returns a potentially invalid JSON object pointer (could be nullptr).
*/
ObjectPtrJSON opt_object(const ObjectPtrJSON& container, const std::string& key);

/*! @brief Returns a potentially invalid JSON array pointer (could be nullptr).
*/
ArrayPtrJSON opt_array(const ObjectPtrJSON& container, const std::string& key);

// TODO: we might want a more general solution around POCO exceptions
template <typename T>
void for_each_in_array(const ArrayPtrJSON& array, std::function<void(T&)> func_over_elem)
{
    rv_assert(array);
    try
    {
        for (const VarJSON& array_item : *array)
        {
            auto object = array_item.extract<T>();  // throws!
            func_over_elem(object);
        }
    }
    catch (const Poco::Exception& e)
    {
        step::utils::throw_runtime_with_log(fmt::format("Iterating over a json array failed with '{}'", e.what()));
    }
}

template <typename T>
void for_first_true_in_array(const ArrayPtrJSON& array, std::function<bool(const T&)> func_over_elem)
{
    rv_assert(array);
    try
    {
        for (const VarJSON& array_item : *array)
        {
            auto object = array_item.extract<T>();  // throws!
            if (func_over_elem(object))
                break;
        }
    }
    catch (const Poco::Exception& e)
    {
        step::utils::throw_runtime_with_log(fmt::format("Iterating over a json array failed with '{}'", e.what()));
    }
}

}  // namespace step::json
