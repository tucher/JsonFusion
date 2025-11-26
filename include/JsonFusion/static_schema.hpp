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

enum class stream_write_result : std::uint8_t {
    slot_allocated,  // one value produced; keep going
    overflow,    // normal end-of-stream
    error,   // unrecoverable error; abort
    value_processed, //normal state
};

namespace static_schema {


namespace input_checks {


template<class T, template<class...> class Template>
struct is_specialization_of : std::false_type {};

template<template<class...> class Template, class... Args>
struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

template<class T, template<class...> class Template>
inline constexpr bool is_specialization_of_v =
    is_specialization_of<std::remove_cvref_t<T>, Template>::value;

// Top-level forbidden shapes: no recursion, no PFR, no ranges.
template<class T>
struct is_directly_forbidden {
    using D = std::remove_cvref_t<T>;
    static constexpr bool value =
        std::is_void_v<D> ||
        std::is_pointer_v<D> ||
        std::is_member_pointer_v<D> ||
        std::is_null_pointer_v<D> ||
        std::is_function_v<D> ||
        std::is_reference_v<T>;
};

template<class T>
inline constexpr bool is_directly_forbidden_v =
    is_directly_forbidden<T>::value;


} // namespace input_checks



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
    array_read_cursor<C>{c}.reset();
};


template<class C>
    requires
    std::ranges::range<C>
struct array_read_cursor<C> {
    using element_type = typename C::value_type;
    const C& c;
    decltype(c.begin()) it = c.begin();
    decltype(c.begin()) b = c.begin();
    bool first = true;
    constexpr const element_type& get() const {
        return *it;
    }
    constexpr stream_read_result read_more() {
        if(first) {
            first = false;
        } else {
            it ++;
        }
        if(it != c.end()) return stream_read_result::value;
        else return stream_read_result::end;
    }
    constexpr void reset() {
        it = b;
        first = true;
    }
};

// fixed-size std::array
template<class T, std::size_t N>
struct array_read_cursor<std::array<T, N>> {
    using element_type = T;
    const std::array<T, N>& c;
    mutable std::size_t index = 0;
    mutable bool first = true;


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
    constexpr void reset() const {
        index = 0;
        first = true;
    }
};



template<class C>
struct array_write_cursor;


template<class C>
concept ArrayWritable = requires(C& c) {
    typename array_write_cursor<C>::element_type;
    { array_write_cursor<C>{c}.allocate_slot() } -> std::same_as<stream_write_result>;
    { array_write_cursor<C>{c}.get_slot() } -> std::same_as<typename array_write_cursor<C>::element_type&>;
    { array_write_cursor<C>{c}.finalize(std::declval<bool>()) } -> std::same_as<stream_write_result>;
    array_write_cursor<C>{c}.reset();

};


template<class C>
struct map_write_cursor;

template<class C>
concept MapWritable = requires(C& c) {
    typename map_write_cursor<C>::key_type;
    typename map_write_cursor<C>::mapped_type;
    { map_write_cursor<C>{c}.allocate_key() } -> std::same_as<stream_write_result>;
    { map_write_cursor<C>{c}.key_ref() } -> std::same_as<typename map_write_cursor<C>::key_type&>;
    { map_write_cursor<C>{c}.allocate_value_for_parsed_key() } -> std::same_as<stream_write_result>;
    { map_write_cursor<C>{c}.value_ref() } -> std::same_as<typename map_write_cursor<C>::mapped_type&>;
    { map_write_cursor<C>{c}.finalize_pair(std::declval<bool>()) } -> std::same_as<stream_write_result>;
    map_write_cursor<C>{c}.reset();
};


template<class C>
struct map_read_cursor;

template<class C>
concept MapReadable = requires(C& c) {
    typename map_read_cursor<C>::key_type;
    typename map_read_cursor<C>::mapped_type;
    { map_read_cursor<C>{c}.read_more() } -> std::same_as<stream_read_result>;
    { map_read_cursor<C>{c}.get_key() } -> std::same_as<const typename map_read_cursor<C>::key_type&>;
    { map_read_cursor<C>{c}.get_value() } -> std::same_as<const typename map_read_cursor<C>::mapped_type&>;
    map_read_cursor<C>{c}.reset();
};

template<class C>
    requires requires(C& c) {
        { c.emplace_back() } -> std::same_as<typename C::value_type & >;
        c.clear();
    }
struct array_write_cursor<C> {
    using element_type = typename C::value_type;
    C& c;

    constexpr stream_write_result allocate_slot() {
        return stream_write_result::slot_allocated;
    }
    constexpr element_type & get_slot() {
        return c.emplace_back();
    }
    constexpr stream_write_result finalize(bool) {
        return  stream_write_result::value_processed;
    }
    constexpr void reset(){
        c.clear();
    }
};

// fixed-size std::array
template<class T, std::size_t N>
struct array_write_cursor<std::array<T, N>> {
    using element_type = T;
    std::array<T, N>& c;
    std::size_t index = 0;
    bool first = true;
    constexpr stream_write_result allocate_slot() {
        if(first) {
            index = 0;
            first = false;
        } else {
            index ++;
        }
        if(index < N)
            return stream_write_result::slot_allocated;
        else {
            return stream_write_result::overflow;
        }
    }
    constexpr element_type & get_slot() {
        return c[index];
    }
    constexpr stream_write_result finalize(bool) {
        return  stream_write_result::value_processed;
    }
    constexpr void reset(){
        index = 0;
        first = true;
    }
};


// Generic map write cursor for map-like containers with try_emplace
template<class M>
    requires requires(M& m) {
        typename M::key_type;
        typename M::mapped_type;
        { m.try_emplace(std::declval<typename M::key_type>(), std::declval<typename M::mapped_type>()) };
        m.clear();
    }
struct map_write_cursor<M> {
    using key_type = typename M::key_type;
    using mapped_type = typename M::mapped_type;
    
    M& m;
    key_type current_key{};
    mapped_type current_value{};
    
    constexpr stream_write_result allocate_key() {
        current_key = key_type{};
        return stream_write_result::slot_allocated;
    }
    
    constexpr key_type& key_ref() {
        return current_key;
    }
    
    constexpr stream_write_result allocate_value_for_parsed_key() {
        current_value = mapped_type{};
        return stream_write_result::slot_allocated;
    }
    
    constexpr mapped_type& value_ref() {
        return current_value;
    }
    
    constexpr stream_write_result finalize_pair(bool ok) {
        if (!ok) return stream_write_result::error;
        
        auto [it, inserted] = m.try_emplace(
            std::move(current_key), 
            std::move(current_value)
        );
        
        return inserted ? stream_write_result::value_processed 
                        : stream_write_result::error;  // duplicate key
    }
    
    constexpr void reset() {
        m.clear();
    }
};


// Generic map read cursor for map-like containers
template<class M>
    requires requires(const M& m) {
        typename M::key_type;
        typename M::mapped_type;
        { m.begin() } -> std::same_as<typename M::const_iterator>;
        { m.end() } -> std::same_as<typename M::const_iterator>;
    }
struct map_read_cursor<M> {
    using key_type = typename M::key_type;
    using mapped_type = typename M::mapped_type;
    
    const M& m;
    typename M::const_iterator it = m.begin();
    typename M::const_iterator b = m.begin();
    bool first = true;
    
    constexpr stream_read_result read_more() {
        if (first) {
            first = false;
        } else {
            ++it;
        }
        return (it != m.end()) ? stream_read_result::value 
                               : stream_read_result::end;
    }
    
    constexpr const key_type& get_key() const {
        return it->first;
    }
    
    constexpr const mapped_type& get_value() const {
        return it->second;
    }
    
    constexpr void reset() {
        it = b;
        first = true;
    }
};



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
        }else if constexpr (ArrayWritable<U>) {
            return false; // arrays are handled separately
        }else if constexpr (MapReadable<U>) {
            return false; // maps are handled separately
        }else if constexpr (MapWritable<U>) {
            return false; // maps are handled separately
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




// helper: detect specializations
template<class T, template<class...> class Template>
struct is_specialization_of : std::false_type {};

template<template<class...> class Template, class... Args>
struct is_specialization_of<Template<Args...>, Template> : std::true_type {};



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
    static constexpr bool value = []{
        if constexpr (JsonBool<T> || JsonNumber<T> || JsonString<T>) {
            return true;
        } else if constexpr (is_json_object<T>::value) {
            return true;
        } else if constexpr (is_json_serializable_array<T>::value) {
            return true;
        } else {
            // Inline map check
            using U = AnnotatedValue<T>;
            if constexpr (MapReadable<U>) {
                using Cursor = map_read_cursor<U>;
                return JsonString<typename Cursor::key_type> &&
                       is_json_serializable_value<typename Cursor::mapped_type>::value;
            } else {
                return false;
            }
        }
    }();
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
concept JsonSerializableValue = !input_checks::is_directly_forbidden_v<C> &&  is_json_serializable_value<C>::value;

template<class T> struct is_json_parsable_value;  // primary declaration
template<class T> struct is_json_parsable_map;     // forward declaration
template<class T> struct is_json_serializable_map; // forward declaration


template<class T>
struct is_json_parsable_array {
    static constexpr bool value = []{
        using U = AnnotatedValue<T>;
        if constexpr (JsonString<T>||JsonBool<T> || JsonNumber<T>)
            return false; //not arrays
        else {
            if constexpr(ArrayWritable<U>) {
                return is_json_parsable_value<typename array_write_cursor<AnnotatedValue<T>>::element_type>::value;
            } else {
                return false;
            }
        }
    }();
};
template<class C>
concept JsonParsableArray = is_json_parsable_array<C>::value;

template<class T>
struct is_non_null_json_parsable_value {
    static constexpr bool value = []{
        if constexpr (JsonBool<T> || JsonNumber<T> || JsonString<T>) {
            return true;
        } else if constexpr (is_json_object<T>::value) {
            return true;
        } else if constexpr (is_json_parsable_array<T>::value) {
            return true;
        } else {
            // Inline map check
            using U = AnnotatedValue<T>;
            if constexpr (MapWritable<U>) {
                using Cursor = map_write_cursor<U>;
                return JsonString<typename Cursor::key_type> &&
                       is_json_parsable_value<typename Cursor::mapped_type>::value;
            } else {
                return false;
            }
        }
    }();
};

template<class Field>
struct is_nullable_json_parsable_value {
    using AV  = AnnotatedValue<Field>; // unwrap Annotated only
    static constexpr bool value = []{
        if constexpr (is_specialization_of<AV, std::optional>::value) {
            using Inner = typename AV::value_type;
            // Inner must itself be a non-null JSON value type
            return is_non_null_json_parsable_value<Inner>::value;
        } else {
            return false;
        }
    }();
};


template<class Field>
concept JsonNullableParsableValue = is_nullable_json_parsable_value<Field>::value;

template<class Field>
concept JsonNonNullableParsableValue = is_non_null_json_parsable_value<Field>::value;


template<class T>
struct is_json_parsable_value {
    static constexpr bool value = is_non_null_json_parsable_value<T>::value
                                  || is_nullable_json_parsable_value<T>::value;
};

template<class C>
concept JsonParsableValue = !input_checks::is_directly_forbidden_v<C> && is_json_parsable_value<C>::value;

/* ######## Map type detection (actual implementations) ######## */

template<typename T>
struct is_json_parsable_map {
    static constexpr bool value = [] {
        using U = AnnotatedValue<T>;
        if constexpr (JsonBool<T> || JsonString<T> || JsonNumber<T>) {
            return false;
        } else if constexpr (is_json_object<T>::value) {
            return false; // objects are handled separately
        } else if constexpr (MapWritable<U>) {
            using Cursor = map_write_cursor<U>;
            return JsonString<typename Cursor::key_type> &&
                   is_json_parsable_value<typename Cursor::mapped_type>::value;
        } else {
            return false;
        }
    }();
};

template<class C>
concept JsonParsableMap = is_json_parsable_map<C>::value;

template<typename T>
struct is_json_serializable_map {
    static constexpr bool value = [] {
        using U = AnnotatedValue<T>;
        if constexpr (JsonBool<T> || JsonString<T> || JsonNumber<T>) {
            return false;
        } else if constexpr (is_json_object<T>::value) {
            return false; // objects are handled separately
        } else if constexpr (MapReadable<U>) {
            using Cursor = map_read_cursor<U>;
            return JsonString<typename Cursor::key_type> &&
                   is_json_serializable_value<typename Cursor::mapped_type>::value;
        } else {
            return false;
        }
    }();
};

template<class C>
concept JsonSerializableMap = is_json_serializable_map<C>::value;

/* ######## Generic data access ######## */


template <JsonNullableParsableValue Field>
constexpr void setNull(Field &f) {
    annotation_meta_getter<Field>::getRef(f).reset();
}

template <JsonNullableSerializableValue Field>
constexpr bool isNull(const Field &f) {
    return !annotation_meta_getter<Field>::getRef(f).has_value();
}

template<JsonNullableParsableValue Field>
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

template<JsonNonNullableParsableValue Field>
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


namespace static_schema {
namespace consuming_streamer_concept_helpers {

template<class S, class U, class V>
consteval bool is_valid_consume_member_type() {
    using M = decltype(&S::consume);

    // Helper aliases for readability
    using C  = S;
    using CU = const U&;
    using CV = const V&;

    // All allowed signatures:
    using SigU      = bool (C::*)(CU);
    using SigUConst = bool (C::*)(CU) const;
    using SigUNoex  = bool (C::*)(CU) noexcept;
    using SigUConstNoex = bool (C::*)(CU) const noexcept;

    using SigV      = bool (C::*)(CV);
    using SigVConst = bool (C::*)(CV) const;
    using SigVNoex  = bool (C::*)(CV) noexcept;
    using SigVConstNoex = bool (C::*)(CV) const noexcept;

    if constexpr (std::is_same_v<U, V>) {
        // No annotation: underlying == value_type
        // Only V-based signatures matter (U and V are same anyway)
        return std::is_same_v<M, SigV> ||
               std::is_same_v<M, SigVConst> ||
               std::is_same_v<M, SigVNoex> ||
               std::is_same_v<M, SigVConstNoex>;
    } else {
        // Annotated case: allow both U- and V-based signatures
        return
            std::is_same_v<M, SigU> ||
            std::is_same_v<M, SigUConst> ||
            std::is_same_v<M, SigUNoex> ||
            std::is_same_v<M, SigUConstNoex> ||

            std::is_same_v<M, SigV> ||
            std::is_same_v<M, SigVConst> ||
            std::is_same_v<M, SigVNoex> ||
            std::is_same_v<M, SigVConstNoex>;
    }
}

template<class S>
consteval bool has_valid_consume_member() {
    // First: does S::consume even exist?
    if constexpr (!requires { &S::consume; }) {
        return false;  // no consume → not a ConsumingStreamerLike
    } else {
        using V = typename S::value_type;
        using U = static_schema::AnnotatedValue<V>;
        return is_valid_consume_member_type<S, U, V>();
    }
}


}
}

//Need to always check custom producers and consumers with this concepts, because by default everything will decay to simple json object
template<class S>
concept ConsumingStreamerLike =
        static_schema::JsonParsableValue<typename S::value_type>
        && static_schema::consuming_streamer_concept_helpers::has_valid_consume_member<S>()
        && requires(S& s) {
            { s.finalize(std::declval<bool>()) } -> std::same_as<bool>;
            { s.reset() } -> std::same_as<void>;
        }
        ;


template<class S>
concept ProducingStreamerLike =
        static_schema::JsonSerializableValue<typename S::value_type>
        && requires(S& s, static_schema::AnnotatedValue<typename S::value_type>& v) {
           //if returns stream_read_result::value, the object was filled and need to continue calling "read".
           // If tream_read_result::value, no more data
           // stream_read_result::error error happened and need to abort serialization
           typename S::value_type;

           { s.read(std::declval<typename S::value_type&>()) } -> std::same_as<stream_read_result>;
           requires (!requires {
               s.read(std::move(v));
           });

           { s.reset() } -> std::same_as<void>;
       };

namespace static_schema {

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

    constexpr void reset() const {
        streamer.reset();
    }
};



// streaming source
template<ConsumingStreamerLike Streamer>
struct array_write_cursor<Streamer> {
    using element_type = Streamer::value_type;
    Streamer & streamer;
    element_type buffer{};
    bool first_read = true;

    constexpr stream_write_result allocate_slot() {
        if(first_read) {
            first_read = false;
            return stream_write_result::slot_allocated;
        }
        if(!streamer.consume(buffer)) {
            return stream_write_result::error;
        } else {
            return stream_write_result::slot_allocated;
        }
    }

    constexpr element_type& get_slot() {
        return buffer;
    }

    constexpr stream_write_result finalize(bool res) {
        if(!streamer.consume(buffer)) {
            return stream_write_result::error;
        }
        if(!streamer.finalize(res)) {
            return stream_write_result::error;
        } else {
            return stream_write_result::value_processed;
        }
    }

    constexpr void reset(){
        first_read = true;
        streamer.reset();
    }
};

namespace detail {
template<class T>
struct always_false : std::false_type {};
}

} //static_schema

} // namespace JsonFusion

