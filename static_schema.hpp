#pragma once
#include <ranges>
#include <type_traits>
#include <string>
#include <string_view>
#include <optional>

namespace JSONReflection2 {

// Forward-declare Annotated
template<typename T, typename... Options>
class Annotated;

namespace static_schema {

template<class T>
using Decay = std::remove_cvref_t<T>;


// Base case: underlying type is itself
template<class T>
struct json_underlying {
    using type = Decay<T>;
};

// Unwrap Annotated<T, ...>
template<typename T, typename... Options>
struct json_underlying<Annotated<T, Options...>> : json_underlying<T> {};

// Unwrap Annotated<T, ...>
template<typename T>
struct json_underlying<std::optional<T>> : json_underlying<T> {};



template<class T>
using JsonUnderlying = typename json_underlying<T>::type;



template<class T> struct is_json_value;  // primary declaration
template<class T> struct is_json_array;  // primary declaration



/* ######## Bool type detection ######## */
template<class C>
concept JsonBool = std::same_as<JsonUnderlying<C>, bool>;

/* ######## Number type detection ######## */
template<class C>
concept JsonNumber =
    !JsonBool<C> &&
    (std::is_integral_v<JsonUnderlying<C>> || std::is_floating_point_v<JsonUnderlying<C>>);

/* ######## String type detection ######## */
template<class C>
concept JsonString =
    std::same_as<JsonUnderlying<C>, std::string>      ||
    std::same_as<JsonUnderlying<C>, std::string_view> ||
    (std::ranges::contiguous_range<JsonUnderlying<C>> &&
     std::same_as<std::ranges::range_value_t<JsonUnderlying<C>>, char>);


/* ######## Object type detection ######## */

template<typename T>
struct is_json_object {
    static constexpr bool value = [] {
        using U = JsonUnderlying<T>;
        if constexpr (JsonBool<T> || JsonString<T> || JsonNumber<T>) {
            return false;
        } else if constexpr (std::ranges::range<U>) {
            return false; // arrays are handled separately
        } else if constexpr (!std::is_class_v<U>) {
            return false;
        } else if constexpr (!std::is_aggregate_v<U>) {
            // static_assert(false, "Type is not aggregate, this is not supported");
            return false;
        } else {
            // Aggregate class, non-scalar, non-string, non-range -> treat as JSON object

            return true;
        }
    }();
};


template<class C>
concept JsonObject = is_json_object<C>::value;



/* ######## Array type detection ######## */

template<class T>
struct is_json_array {
    static constexpr bool value = []{
        using U = JsonUnderlying<T>;
        if constexpr (!std::ranges::range<U>)
            return false;
        else if constexpr (JsonString<T>)
            return false; // strings are not arrays
        else {
            using Elem = std::ranges::range_value_t<U>;
            return is_json_value<Elem>::value;  // recursion over element type
        }
    }();
};

template<class C>
concept JsonArray = is_json_array<C>::value;



/* ######## Generic JSON value detection ######## */

template<class T>
struct is_non_null_json_value {
    static constexpr bool value =
        JsonBool<T>   ||
        JsonNumber<T> ||
        JsonString<T> ||
        is_json_object<T>::value ||
        is_json_array<T>::value;
};


template<class T>
struct is_nullable_json_value_impl : std::false_type {};

template<class U>
struct is_nullable_json_value_impl<std::optional<U>>
    : std::bool_constant<is_non_null_json_value<U>::value> {};

template<class T>
struct is_nullable_json_value
    : is_nullable_json_value_impl<Decay<T>> {};

template<class T>
struct is_json_value {
    static constexpr bool value = is_non_null_json_value<T>::value
                               || is_nullable_json_value<T>::value;
};

template<class C>
concept JsonNullableValue = is_nullable_json_value<C>::value;


template<class C>
concept JsonValue = is_json_value<C>::value;



} // namespace static_schema

} // namespace JSONReflection2

