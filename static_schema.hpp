#pragma once
#include <ranges>
#include <type_traits>
#include <string>
#include <string_view>
#include <optional>

#include "options.hpp"
namespace JSONReflection2 {


namespace static_schema {


using options::detail::annotation_meta_getter;

template<class Field>
using AnnotatedValue = typename annotation_meta_getter<Field>::value_t;


template<class T> struct is_json_value;  // primary declaration
template<class T> struct is_json_array;  // primary declaration


/* ######## Bool type detection ######## */
template<class C>
concept JsonBool = std::same_as<AnnotatedValue<C>, bool>;

/* ######## Number type detection ######## */
template<class C>
concept JsonNumber =
    !JsonBool<C> &&
    (std::is_integral_v<AnnotatedValue<C>> || std::is_floating_point_v<AnnotatedValue<C>>);

/* ######## String type detection ######## */
template<class C>
concept JsonString =
    std::same_as<AnnotatedValue<C>, std::string>      ||
    std::same_as<AnnotatedValue<C>, std::string_view> ||
    (std::ranges::contiguous_range<AnnotatedValue<C>> &&
     std::same_as<std::ranges::range_value_t<AnnotatedValue<C>>, char>);


/* ######## Object type detection ######## */

template<typename T>
struct is_json_object {
    static constexpr bool value = [] {
        using U = AnnotatedValue<T>;
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
        using U = AnnotatedValue<T>;
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

// helper: detect specializations
template<class T, template<class...> class Template>
struct is_specialization_of : std::false_type {};

template<template<class...> class Template, class... Args>
struct is_specialization_of<Template<Args...>, Template> : std::true_type {};


template<class Field>
struct is_nullable_json_value {
    using AV  = AnnotatedValue<Field>; // unwrap Annotated only
    static constexpr bool value = []{
        if constexpr (is_specialization_of<AV, std::optional>::value) {
            using Inner = typename AV::value_type;
            // Inner must itself be a non-null JSON value type
            return is_non_null_json_value<Inner>::value;
        } else {
            return false;
        }
    }();
};

template<class Field>
concept JsonNullableValue = is_nullable_json_value<Field>::value;

template<class Field>
concept JsonNonNullableValue = is_non_null_json_value<Field>::value;


template<class T>
struct is_json_value {
    static constexpr bool value = is_non_null_json_value<T>::value
                               || is_nullable_json_value<T>::value;
};


template<class C>
concept JsonValue = is_json_value<C>::value;


/* ######## Generic data access ######## */


template <JsonNullableValue Field>
void setNull(Field &f) {
    annotation_meta_getter<Field>::getRef(f).reset();
}

template <JsonNullableValue Field>
static bool isNull(const Field &f) {
    return !annotation_meta_getter<Field>::getRef(f).has_value();
}

template<JsonNullableValue Field>
decltype(auto) getRef(Field & f) {
    using S = annotation_meta_getter<Field>;
    auto& opt = S::getRef(f);
    if(!opt) return opt.emplace();
    else return *opt;
}

template<JsonNullableValue Field>
decltype(auto) getRef(const Field & f) { // This must be used only after checking for null with isNull
    using S = annotation_meta_getter<Field>;
    return (*S::getRef(f));
}

template<JsonNonNullableValue Field>
decltype(auto) getRef(Field & f) {
    using S = annotation_meta_getter<Field>;
    return (S::getRef(f));
}

template<JsonNonNullableValue Field>
decltype(auto) getRef(const Field & f) {
    using S = annotation_meta_getter<Field>;
    return (S::getRef(f));
}



} // namespace static_schema

} // namespace JSONReflection2

