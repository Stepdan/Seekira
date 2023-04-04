#pragma once

#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>

#include <string_view>

template <typename T>
using PtrJSON = Poco::SharedPtr<T>;

using ObjectJSON = Poco::JSON::Object;
using ObjectPtrJSON = PtrJSON<ObjectJSON>;

using ArrayJSON = Poco::JSON::Array;
using ArrayPtrJSON = PtrJSON<ArrayJSON>;

using VarJSON = Poco::Dynamic::Var;

// Content types
namespace step::json::content_type {
constexpr std::string_view application_json = "application/json";
}  // namespace step::json::content_type
