#pragma once
#include <ranges>
#include <type_traits>
#include <string>
#include <string_view>
#include <optional>
#include <utility>

#include "options.hpp"
namespace JsonFusion {

enum class stream_read_result : std::uint8_t {
    value,  // one value produced; keep going
    end,    // normal end-of-stream
    error   // unrecoverable error; abort
};

namespace static_schema {


using options::detail::annotation_meta_getter;

template<class Field>
using AnnotatedValue = typename annotation_meta_getter<Field>::value_t;


template<class C>
struct array_read_cursor{};


template<class C>
concept ArrayReadable = requires(C& c) {
    typename array_read_cursor<C>::element_type;
    { array_read_cursor<C>{c}.read_more() } -> std::same_as<stream_read_result>;
    { array_read_cursor<C>{c}.get() } -> std::same_as<const typename array_read_cursor<C>::element_type&>;
};


template<class C>
    requires
    std::ranges::range<C>
struct array_read_cursor<C> {
    using element_type = typename C::value_type;
    const C& c;
    decltype(c.begin()) it = c.begin();
    bool first = true;
    const element_type& get() const {
        return *it;
    }
    stream_read_result read_more() {
        if(first) {
            first = false;
        } else {
            it ++;
        }
        if(it != c.end()) return stream_read_result::value;
        else return stream_read_result::end;
    }
};

// fixed-size std::array
template<class T, std::size_t N>
struct array_read_cursor<std::array<T, N>> {
    using element_type = T;
    const std::array<T, N>& c;
    std::size_t index = 0;
    bool first = true;


    constexpr const element_type& get() {
        return c[index];
    }
    constexpr stream_read_result read_more() {
        if(first) {
            first = false;
        } else  {
            index ++;
        }
        if(index < N) return stream_read_result::value;
        else return stream_read_result::end;
    }
};



template<class S>
concept ProducingStreamerLike = requires(S& s) {
    //if return true, the the object was filled with data and need to continue calling "read". If false, no more data or error
    typename S::value_type;
    { s.read(std::declval<typename S::value_type&>()) } -> std::same_as<stream_read_result>;
};


// streaming source
template<ProducingStreamerLike Streamer>
struct array_read_cursor<Streamer> {
    using element_type = Streamer::value_type;
    const Streamer & streamer;
    element_type buffer;
    bool first_read = false;
    bool has_more = false;
    constexpr const element_type& get() {
        return buffer;
    }
    constexpr stream_read_result read_more() {
        return streamer.read(buffer);
    }
};



template<class C>
struct array_write_cursor;


template<class C>
concept ArrayWritable = requires(C& c) {
    typename array_write_cursor<C>::element_type;
    { array_write_cursor<C>{c}.has_space() } -> std::same_as<bool>;
    { array_write_cursor<C>{c}.emplace() } -> std::same_as<typename array_write_cursor<C>::element_type&>;
};




template<class C>
    requires requires(C& c, typename C::value_type v) { c.emplace_back(v); }
struct array_write_cursor<C> {
    using element_type = typename C::value_type;
    C& c;

    element_type* next_slot() {
        auto& ref = c.emplace_back();
        return std::addressof(ref);
    }
};

// fixed-size std::array
template<class T, std::size_t N>
struct array_write_cursor<std::array<T, N>> {
    using element_type = T;
    std::array<T, N>& c;
    std::size_t index = 0;

    element_type* next_slot() {
        if (index < N) return std::addressof(c[index++]);
        return nullptr;
    }
};


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
        }else if constexpr (ArrayReadable<U>) {
            return false; // arrays are handled separately
        }else if constexpr (!std::is_class_v<U>) {
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





template<class T> struct is_json_serializable_value;  // primary declaration



template<class T>
struct is_json_serializable_array {
    static constexpr bool value = []{
        using U = AnnotatedValue<T>;
        if constexpr (JsonString<T>||JsonBool<T> || JsonNumber<T>)
            return false; //not arrays
        else {
            if constexpr(ArrayReadable<U>) {
                return is_json_serializable_value<typename array_read_cursor<AnnotatedValue<T>>::element_type>::value;
            } else {
                return false;
            }
        }
    }();
};

template<class C>
concept JsonSerializableArray = is_json_serializable_array<C>::value;
template<class T>
struct is_non_null_json_serializable_value {
    static constexpr bool value =
        JsonBool<T>   ||
        JsonNumber<T> ||
        JsonString<T> ||
        is_json_object<T>::value ||
        is_json_serializable_array<T>::value;
};



template<class Field>
struct is_nullable_json_serializable_value {
    using AV  = AnnotatedValue<Field>; // unwrap Annotated only
    static constexpr bool value = []{
        if constexpr (is_specialization_of<AV, std::optional>::value) {
            using Inner = typename AV::value_type;
            // Inner must itself be a non-null JSON value type
            return is_non_null_json_serializable_value<Inner>::value;
        } else {
            return false;
        }
    }();
};




template<class Field>
concept JsonNullableSerializableValue = is_nullable_json_serializable_value<Field>::value;

template<class Field>
concept JsonNonNullableSerializableValue = is_non_null_json_serializable_value<Field>::value;


template<class T>
struct is_json_serializable_value {
    static constexpr bool value = is_non_null_json_serializable_value<T>::value
                                  || is_nullable_json_serializable_value<T>::value;
};

template<class C>
concept JsonSerializableValue = is_json_serializable_value<C>::value;




/* ######## Generic data access ######## */


template <JsonNullableValue Field>
constexpr void setNull(Field &f) {
    annotation_meta_getter<Field>::getRef(f).reset();
}

template <JsonNullableSerializableValue Field>
constexpr bool isNull(const Field &f) {
    return !annotation_meta_getter<Field>::getRef(f).has_value();
}

template<JsonNullableValue Field>
constexpr decltype(auto) getRef(Field & f) {
    using S = annotation_meta_getter<Field>;
    auto& opt = S::getRef(f);
    if(!opt) return opt.emplace();
    else return *opt;
}

template<JsonNullableSerializableValue Field>
constexpr decltype(auto) getRef(const Field & f) { // This must be used only after checking for null with isNull
    using S = annotation_meta_getter<Field>;
    return (*S::getRef(f));
}

template<JsonNonNullableValue Field>
constexpr decltype(auto) getRef(Field & f) {
    using S = annotation_meta_getter<Field>;
    return (S::getRef(f));
}

template<JsonNonNullableSerializableValue Field>
constexpr decltype(auto) getRef(const Field & f) {
    using S = annotation_meta_getter<Field>;
    return (S::getRef(f));
}



} // namespace static_schema

} // namespace JsonFusion

