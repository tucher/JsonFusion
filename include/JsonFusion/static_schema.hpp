#pragma once
#include <ranges>
#include <type_traits>
#if __has_include(<string>)
#include <string>
#endif
#include <string_view>
#include <optional>
#include <utility>
#include <algorithm>


#include "options.hpp"
#include "struct_introspection.hpp"
#include "wire_sink.hpp"

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
constexpr bool is_specialization_of_v =
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
constexpr bool is_directly_forbidden_v =
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
    { array_read_cursor<C>{c}.size() } -> std::same_as<std::size_t>;

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

    constexpr std::size_t size() {
        return c.size();
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
    constexpr std::size_t size() {
        return N;
    }
};

// fixed-size plain array
template<class T, std::size_t N>
struct array_read_cursor<T[N]> {
    using element_type = T;
    const T(&c)[N];
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
    constexpr std::size_t size() {
        return N;
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
    { array_write_cursor<C>{c}.finalize_item(std::declval<bool>()) } -> std::same_as<stream_write_result>;

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
    { map_write_cursor<C>{c}.finalize(std::declval<bool>()) } -> std::same_as<stream_write_result>;
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
    { map_read_cursor<C>{c}.size() } -> std::same_as<std::size_t>;

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
    constexpr stream_write_result finalize_item(bool) {
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
    constexpr stream_write_result finalize_item(bool ok) {
        return  stream_write_result::value_processed;
    }
    constexpr void reset(){
        index = 0;
        first = true;
    }
};

// fixed-size plain array
template<class T, std::size_t N>
struct array_write_cursor<T[N]> {
    using element_type = T;
    T (&c)[N];
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
    constexpr stream_write_result finalize_item(bool ok) {
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
                        : stream_write_result::overflow;  // duplicate key
    }
    constexpr stream_write_result finalize(bool) {
        return  stream_write_result::value_processed;
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
    constexpr std::size_t size() {
        return m.size();
    }
};


/* ######## String cursor infrastructure ######## */

// Primary templates (empty - specializations provide the implementation)
template<class C>
struct string_read_cursor {};

template<class C>
struct string_write_cursor {};

// Concept: type supports reading string content for serialization
// Symmetric with ArrayReadable - uses read_more() pattern
template<class C>
concept StringReadable = requires(const C& c) {
    typename string_read_cursor<C>::char_type;
    requires std::is_constructible_v<string_read_cursor<C>, const C&>;
    { std::declval<string_read_cursor<C>>().read_more() } -> std::same_as<stream_read_result>;
    { std::declval<string_read_cursor<C>>().data() } -> std::same_as<const char*>;
    { std::declval<string_read_cursor<C>>().size() } -> std::same_as<std::size_t>;
    { std::declval<string_read_cursor<C>>().total_size() } -> std::same_as<std::size_t>;
    std::declval<string_read_cursor<C>>().reset();
};

// Concept: type supports writing string content during parsing
template<class C>
concept StringWritable = requires(C& c, std::size_t n) {
    typename string_write_cursor<C>::char_type;
    requires std::is_constructible_v<string_write_cursor<C>, C&>;
    { std::declval<string_write_cursor<C>>().prepare_write(n) } -> std::same_as<std::size_t>;
    { std::declval<string_write_cursor<C>>().write_ptr() } -> std::same_as<char*>;
    std::declval<string_write_cursor<C>>().commit(n);
    std::declval<string_write_cursor<C>>().finalize();
    { std::declval<string_write_cursor<C>>().size() } -> std::same_as<std::size_t>;
    { std::declval<string_write_cursor<C>>().max_capacity() } -> std::same_as<std::size_t>;
    { std::declval<const string_write_cursor<C>>().view() } -> std::same_as<std::string_view>;
    std::declval<string_write_cursor<C>>().reset();
};

// Specialization: std::array<char, N> - fixed size buffer
// For reading: single chunk containing the whole string (zero-copy)
template<std::size_t N>
struct string_read_cursor<std::array<char, N>> {
    using char_type = char;
    
private:
    const std::array<char, N>& arr_;
    mutable bool done_ = false;
    mutable std::size_t len_ = 0;
    
public:
    constexpr explicit string_read_cursor(const std::array<char, N>& a) : arr_(a) {}
    
    constexpr stream_read_result read_more() const {
        if (done_) return stream_read_result::end;
        // Calculate length by finding null terminator (or use full array)
        len_ = 0;
        while (len_ < N && arr_[len_] != '\0') {
            ++len_;
        }
        done_ = true;
        return stream_read_result::value;
    }
    constexpr const char* data() const { return arr_.data(); }
    constexpr std::size_t size() const { return len_; }
    constexpr std::size_t total_size() const { return len_; }
    constexpr void reset() const { done_ = false; len_ = 0; }
};

// For writing: direct buffer access (zero-copy)
template<std::size_t N>
struct string_write_cursor<std::array<char, N>> {
    using char_type = char;
    
private:
    std::array<char, N>& arr_;
    std::size_t pos_ = 0;
    
public:
    constexpr explicit string_write_cursor(std::array<char, N>& a) : arr_(a) {}
    
    constexpr std::size_t prepare_write(std::size_t hint) {
        std::size_t remaining = max_capacity() - pos_;
        return hint < remaining ? hint : remaining;
    }
    constexpr char* write_ptr() { return arr_.data() + pos_; }
    constexpr void commit(std::size_t n) { pos_ += n; }
    constexpr void reset() { pos_ = 0; }
    constexpr void finalize() { if (pos_ < N) arr_[pos_] = '\0'; }
    constexpr std::size_t size() const { return pos_; }
    constexpr std::size_t max_capacity() const { return N > 0 ? N - 1 : 0; }
    constexpr std::string_view view() const { return std::string_view(arr_.data(), pos_); }
};

// Specialization: char[N] - fixed size C-style array
// For reading: single chunk (zero-copy)
template<std::size_t N>
struct string_read_cursor<char[N]> {
    using char_type = char;
    
private:
    const char (&arr_)[N];
    mutable bool done_ = false;
    mutable std::size_t len_ = 0;
    
public:
    constexpr explicit string_read_cursor(const char (&a)[N]) : arr_(a) {}
    
    constexpr stream_read_result read_more() const {
        if (done_) return stream_read_result::end;
        // Calculate length by finding null terminator (or use full array)
        len_ = 0;
        while (len_ < N && arr_[len_] != '\0') {
            ++len_;
        }
        done_ = true;
        return stream_read_result::value;
    }
    constexpr const char* data() const { return arr_; }
    constexpr std::size_t size() const { return len_; }
    constexpr std::size_t total_size() const { return len_; }
    constexpr void reset() const { done_ = false; len_ = 0; }
};

// For writing: direct buffer access (zero-copy)
template<std::size_t N>
struct string_write_cursor<char[N]> {
    using char_type = char;
    
private:
    char (&arr_)[N];
    std::size_t pos_ = 0;
    
public:
    constexpr explicit string_write_cursor(char (&a)[N]) : arr_(a) {}
    
    constexpr std::size_t prepare_write(std::size_t hint) {
        std::size_t remaining = max_capacity() - pos_;
        return hint < remaining ? hint : remaining;
    }
    constexpr char* write_ptr() { return arr_ + pos_; }
    constexpr void commit(std::size_t n) { pos_ += n; }
    constexpr void reset() { pos_ = 0; }
    constexpr void finalize() { if (pos_ < N) arr_[pos_] = '\0'; }
    constexpr std::size_t size() const { return pos_; }
    constexpr std::size_t max_capacity() const { return N > 0 ? N - 1 : 0; }
    constexpr std::string_view view() const { return std::string_view(arr_, pos_); }
};

// Specialization: std::string_view - read only (for serialization)
template<>
struct string_read_cursor<std::string_view> {
    using char_type = char;
    
private:
    std::string_view sv_;
    mutable bool done_ = false;
    
public:
    constexpr explicit string_read_cursor(std::string_view s) : sv_(s) {}
    
    constexpr stream_read_result read_more() const {
        if (done_) return stream_read_result::end;
        done_ = true;
        return stream_read_result::value;
    }
    constexpr const char* data() const { return sv_.data(); }
    constexpr std::size_t size() const { return sv_.size(); }
    constexpr std::size_t total_size() const { return sv_.size(); }
    constexpr void reset() const { done_ = false; }
};

#if __has_include(<string>)
// Specialization: std::string - dynamic size
// For reading: single chunk (zero-copy)
template<>
struct string_read_cursor<std::string> {
    using char_type = char;
    
private:
    const std::string& str_;
    mutable bool done_ = false;
    
public:
    constexpr explicit string_read_cursor(const std::string& s) : str_(s) {}
    
    constexpr stream_read_result read_more() const {
        if (done_) return stream_read_result::end;
        done_ = true;
        return stream_read_result::value;
    }
    constexpr const char* data() const { return str_.data(); }
    constexpr std::size_t size() const { return str_.size(); }
    constexpr std::size_t total_size() const { return str_.size(); }
    constexpr void reset() const { done_ = false; }
};

// For writing: uses internal temp buffer + append (optimal for unknown-size strings)
// Buffer size is implementation detail, not exposed
template<>
struct string_write_cursor<std::string> {
    using char_type = char;
    
private:
    std::string& str_;
    static constexpr std::size_t BUFFER_SIZE = 64;  // implementation detail
    char buffer_[BUFFER_SIZE];
    
public:
    constexpr explicit string_write_cursor(std::string& s) : str_(s), buffer_{} {}
    
    constexpr std::size_t prepare_write(std::size_t hint) {
        return hint < BUFFER_SIZE ? hint : BUFFER_SIZE;
    }
    constexpr char* write_ptr() { return buffer_; }
    constexpr void commit(std::size_t n) { str_.append(buffer_, n); }
    constexpr void reset() { str_.clear(); }
    constexpr void finalize() { /* nothing needed */ }
    constexpr std::size_t size() const { return str_.size(); }
    constexpr std::size_t max_capacity() const { return std::numeric_limits<std::size_t>::max(); }
    constexpr std::string_view view() const { return std::string_view(str_); }
};
#endif


template<class T, class = void>
struct parse_transform_traits;

template<class T, class = void>
struct serialize_transform_traits;


/* ######## Bool type detection ######## */
template<class C>
concept BoolLike = std::same_as<AnnotatedValue<C>, bool>;

/* ######## Number type detection ######## */
template<class C>
concept NumberLike =
    !BoolLike<C> &&
    (std::is_integral_v<AnnotatedValue<C>> || std::is_floating_point_v<AnnotatedValue<C>>);

/* ######## String type detection ######## */

// ParsableStringLike: type can receive string data during parsing (via string_write_cursor)
template<class C>
concept ParsableStringLike =
    !WireSinkLike<AnnotatedValue<C>> &&
    !parse_transform_traits<C>::is_transformer &&
    StringWritable<AnnotatedValue<C>>;

// SerializableStringLike: type can provide string data for serialization (via string_read_cursor)
template<class C>
concept SerializableStringLike =
    !WireSinkLike<AnnotatedValue<C>> &&
    !serialize_transform_traits<C>::is_transformer &&
    StringReadable<AnnotatedValue<C>>;

// Legacy alias for compatibility during migration - will be removed
template<class C>
concept StringLike = ParsableStringLike<C> || SerializableStringLike<C>;








/* ######## Object type detection ######## */

template<typename T>
struct is_json_object {
    static constexpr bool value = [] {
        using U = AnnotatedValue<T>;
        if constexpr (WireSinkLike<U>) {
            return false; // WireSink is NOT an object
        } else if constexpr (BoolLike<T> || ParsableStringLike<T> || SerializableStringLike<T> || NumberLike<T>) {
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
        }else if constexpr (parse_transform_traits<U>::is_transformer || serialize_transform_traits<U>::is_transformer){
            return false;
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
concept ObjectLike = is_json_object<C>::value;




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
        if constexpr (WireSinkLike<U>) {
            return false; // WireSink is NOT an array
        } else if constexpr (SerializableStringLike<T>||BoolLike<T> || NumberLike<T>)
            return false; //not arrays
        else if constexpr(serialize_transform_traits<U>::is_transformer) {
            return false;
        }else {
            if constexpr(ArrayReadable<U>) {
                return is_json_serializable_value<typename array_read_cursor<AnnotatedValue<T>>::element_type>::value;
            } else {
                return false;
            }
        }
    }();
};

template<class C>
concept SerializableArrayLike = is_json_serializable_array<C>::value;

template<class T>
struct is_non_null_json_serializable_value {
    static constexpr bool value = []{
        using U = AnnotatedValue<T>;
        if constexpr (WireSinkLike<U>) {
            return false; // WireSink is NOT a regular non-null value
        } else if constexpr (BoolLike<T> || NumberLike<T> || SerializableStringLike<T>) {
            return true;
        } else if constexpr (is_json_object<T>::value) {
            return true;
        } else if constexpr (is_json_serializable_array<T>::value) {
            return true;
        } else {
            // Inline map check
            if constexpr (MapReadable<U>) {
                using Cursor = map_read_cursor<U>;
                return (SerializableStringLike<typename Cursor::key_type> || std::integral<typename Cursor::key_type>) &&
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
        } else if constexpr (is_specialization_of<AV, std::unique_ptr>::value) {
            using Inner = typename AV::element_type;
            // Inner must itself be a non-null JSON value type
            return is_non_null_json_serializable_value<Inner>::value;
        } else {
            return false;
        }
    }();
};


template<class Field>
concept NullableSerializableValue = is_nullable_json_serializable_value<Field>::value||
                                        (serialize_transform_traits<Field>::is_transformer && is_nullable_json_serializable_value<typename serialize_transform_traits<Field>::wire_type>::value);

template<class Field>
concept NonNullableSerializableValue = is_non_null_json_serializable_value<Field>::value ||
                                           (serialize_transform_traits<Field>::is_transformer && is_non_null_json_serializable_value<typename serialize_transform_traits<Field>::wire_type>::value);


template<class T>
struct is_json_serializable_value {
    static constexpr bool value = WireSinkLike<AnnotatedValue<T>>  // WireSink IS serializable!
                                  || is_non_null_json_serializable_value<T>::value
                                  || is_nullable_json_serializable_value<T>::value 
                                  || serialize_transform_traits<T>::is_transformer;
};

template<class C>
concept SerializableValue = !input_checks::is_directly_forbidden_v<C> &&  is_json_serializable_value<C>::value;

template<class T> struct is_json_parsable_value;  // primary declaration


template<class T>
struct is_json_parsable_array {
    static constexpr bool value = []{
        using U = AnnotatedValue<T>;
        if constexpr (WireSinkLike<U>) {
            return false; // WireSink is NOT an array
        } else if constexpr (ParsableStringLike<T>||BoolLike<T> || NumberLike<T>)
            return false; //not arrays
        else {
            if constexpr(ArrayWritable<U>) {
                return is_json_parsable_value<typename array_write_cursor<AnnotatedValue<T>>::element_type>::value;
            }if constexpr(parse_transform_traits<U>::is_transformer) {
                return false;
            } else {
                return false;
            }
        }
    }();
};
template<class C>
concept ParsableArrayLike = is_json_parsable_array<C>::value;

template<class T>
struct is_non_null_json_parsable_value {
    static constexpr bool value = []{
        using U = AnnotatedValue<T>;
        if constexpr (WireSinkLike<U>) {
            return false; // WireSink is NOT a regular non-null value
        } else if constexpr (BoolLike<T> || NumberLike<T> || ParsableStringLike<T>) {
            return true;
        } else if constexpr (is_json_object<T>::value) {
            return true;
        } else if constexpr (is_json_parsable_array<T>::value) {
            return true;
        } else {
            // Inline map check
            if constexpr (MapWritable<U>) {
                using Cursor = map_write_cursor<U>;
                return (ParsableStringLike<typename Cursor::key_type> || std::integral<typename Cursor::key_type>) &&
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
        } else if constexpr (is_specialization_of<AV, std::unique_ptr>::value) {
            using Inner = typename AV::element_type;
            // Inner must itself be a non-null JSON value type
            return is_non_null_json_parsable_value<Inner>::value;
        } else {
            return false;
        }
    }();
};


template<class Field>
concept NullableParsableValue = is_nullable_json_parsable_value<Field>::value||
                                    (parse_transform_traits<Field>::is_transformer && is_nullable_json_parsable_value<typename parse_transform_traits<Field>::wire_type>::value);


template<class Field>
concept NonNullableParsableValue = is_non_null_json_parsable_value<Field>::value||
                                       (parse_transform_traits<Field>::is_transformer && is_non_null_json_parsable_value<typename parse_transform_traits<Field>::wire_type>::value);



template<class T>
struct is_json_parsable_value {
    static constexpr bool value = WireSinkLike<AnnotatedValue<T>>  // WireSink IS parsable!
                                  || is_non_null_json_parsable_value<T>::value
                                  || is_nullable_json_parsable_value<T>::value 
                                  || parse_transform_traits<T>::is_transformer;
};

template<class C>
concept ParsableValue = !input_checks::is_directly_forbidden_v<C> && is_json_parsable_value<C>::value;

/* ######## Map type detection (actual implementations) ######## */

template<typename T>
struct is_json_parsable_map {
    static constexpr bool value = [] {
        using U = AnnotatedValue<T>;
        if constexpr (WireSinkLike<U>) {
            return false; // WireSink is NOT a map
        } else if constexpr (BoolLike<T> || ParsableStringLike<T> || NumberLike<T>) {
            return false;
        } else if constexpr (is_json_object<T>::value) {
            return false; // objects are handled separately
        } else if constexpr (MapWritable<U>) {
            using Cursor = map_write_cursor<U>;
            return (ParsableStringLike<typename Cursor::key_type> || std::integral<typename Cursor::key_type>) &&
                   is_json_parsable_value<typename Cursor::mapped_type>::value;
        } else {
            return false;
        }
    }();
};

template<class C>
concept ParsableMapLike = is_json_parsable_map<C>::value;

template<typename T>
struct is_json_serializable_map {
    static constexpr bool value = [] {
        using U = AnnotatedValue<T>;
        if constexpr (WireSinkLike<U>) {
            return false; // WireSink is NOT a map
        } else if constexpr (BoolLike<T> || SerializableStringLike<T> || NumberLike<T>) {
            return false;
        } else if constexpr (is_json_object<T>::value) {
            return false; // objects are handled separately
        } else if constexpr (MapReadable<U>) {
            using Cursor = map_read_cursor<U>;
            return (SerializableStringLike<typename Cursor::key_type> || std::integral<typename Cursor::key_type>) &&
                   is_json_serializable_value<typename Cursor::mapped_type>::value;
        } else {
            return false;
        }
    }();
};

template<class C>
concept SerializableMapLike = is_json_serializable_map<C>::value;

/* ######## Generic data access ######## */


template <NullableParsableValue Field>
constexpr void setNull(Field &f) {
    annotation_meta_getter<Field>::getRef(f).reset(); // same for std::optional and std::unique_ptr
}

template <NullableSerializableValue Field>
constexpr bool isNull(const Field &f) {
    if constexpr (is_specialization_of<Field, std::optional>::value) {
        return !annotation_meta_getter<Field>::getRef(f).has_value();
    } else if constexpr (is_specialization_of<Field, std::unique_ptr>::value) {
        return annotation_meta_getter<Field>::getRef(f).get() == nullptr;
    } else {
        static_assert(false, "NullableSerializableValue must be std::optional or std::unique_ptr");
        return false;
    }
}

template<NullableParsableValue Field>
constexpr decltype(auto) getRef(Field & f) {
    using S = annotation_meta_getter<Field>;
    if constexpr (is_specialization_of<typename S::value_t, std::optional>::value) {
        auto& opt = S::getRef(f);
        if(!opt) 
            return opt.emplace();
        else return *opt;
    } else if constexpr (is_specialization_of<typename S::value_t, std::unique_ptr>::value) {
        auto& opt = S::getRef(f);
        if(opt == nullptr)
            opt = std::make_unique<typename S::value_t::element_type>();
        return *opt;
    } else {
        static_assert(false, "NullableParsableValue must be std::optional or std::unique_ptr");
        return false;
    }
}

template<NullableSerializableValue Field>
constexpr decltype(auto) getRef(const Field & f) { // This must be used only after checking for null with isNull
    using S = annotation_meta_getter<Field>;
    return (*S::getRef(f));
}

template<NonNullableParsableValue Field>
constexpr decltype(auto) getRef(Field & f) {
    using S = annotation_meta_getter<Field>;
    return (S::getRef(f));
}

template<NonNullableSerializableValue Field>
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
        static_schema::ParsableValue<typename S::value_type>
        && static_schema::consuming_streamer_concept_helpers::has_valid_consume_member<S>()
        && requires(S& s) {
            { s.finalize(std::declval<bool>()) } -> std::same_as<bool>;
            { s.reset() } -> std::same_as<void>;
        }
        // Exclude map streamers (which have .key and .value in value_type)
        && !requires {
            { std::declval<typename S::value_type>().key };
            { std::declval<typename S::value_type>().value };
        };


template<class S>
concept ProducingStreamerLike =
        static_schema::SerializableValue<typename S::value_type>
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
        }
        // Exclude map streamers (which have .key and .value in value_type)
        && !requires {
            { std::declval<typename S::value_type>().key };
            { std::declval<typename S::value_type>().value };
        };

// High-level map consumer interface (for parsing)
// value_type should be a struct/pair with .key and .value members
template<class S>
concept ConsumingMapStreamerLike =
        requires(S& s, const typename S::value_type& entry) {
            typename S::value_type;
            // Check that value_type has .key and .value members
            { std::declval<typename S::value_type>().key };
            { std::declval<typename S::value_type>().value };
            // Methods
            { s.consume(entry) } -> std::same_as<bool>;
            { s.finalize(std::declval<bool>()) } -> std::same_as<bool>;
            { s.reset() } -> std::same_as<void>;
        }
        // Key must be string-like (parsable), value must be parsable
        && static_schema::ParsableStringLike<decltype(std::declval<typename S::value_type>().key)>
        && static_schema::ParsableValue<decltype(std::declval<typename S::value_type>().value)>;

// High-level map producer interface (for serialization)
// value_type should be a struct/pair with .key and .value members
template<class S>
concept ProducingMapStreamerLike =
        requires(S& s, typename S::value_type& entry) {
            typename S::value_type;
            // Check that value_type has .key and .value members
            { std::declval<typename S::value_type>().key };
            { std::declval<typename S::value_type>().value };
            // Methods
            { s.read(entry) } -> std::same_as<stream_read_result>;
            { s.reset() } -> std::same_as<void>;
        }
        // Key must be string-like (serializable), value must be serializable
        && static_schema::SerializableStringLike<decltype(std::declval<typename S::value_type>().key)>
        && static_schema::SerializableValue<decltype(std::declval<typename S::value_type>().value)>;

namespace static_schema {

template<class Streamer, class Ctx>
constexpr void streamer_context_setter(const Streamer & s, Ctx * ctx) {
    if constexpr( requires{s.set_jsonfusion_context(ctx);} ) {
        s.set_jsonfusion_context(ctx);
    }
}

template<class Streamer, class Ctx>
constexpr void streamer_context_setter(Streamer & s, Ctx * ctx) {
    if constexpr( requires{s.set_jsonfusion_context(ctx);} ) {
        s.set_jsonfusion_context(ctx);
    }
}

// streaming source
template<ProducingStreamerLike Streamer>
struct array_read_cursor<Streamer> {
    using element_type = Streamer::value_type;
    const Streamer & streamer;
    element_type buffer;
    bool first_read = false;
    bool has_more = false;

    constexpr array_read_cursor(const Streamer& s)
        : streamer(s)
        , buffer{} {}

    template<class Ctx>
    constexpr array_read_cursor(const Streamer& s, Ctx * ctx)
        : streamer(s)
        , buffer{} {
        streamer_context_setter(s, ctx);
    }

    constexpr const element_type& get() {
        return buffer;
    }
    constexpr stream_read_result read_more() {
        return streamer.read(buffer);
    }

    constexpr void reset() const {
        streamer.reset();
    }
    constexpr std::size_t size() {
        return -1;
    }
};



// streaming source
template<ConsumingStreamerLike Streamer>
struct array_write_cursor<Streamer> {
    using element_type = Streamer::value_type;
    Streamer & streamer;
    element_type buffer{};

    constexpr array_write_cursor(Streamer& s)
        : streamer(s)
        , buffer{} {}

    template<class Ctx>
    constexpr array_write_cursor(Streamer& s, Ctx * ctx)
        : streamer(s)
        , buffer{} {
        streamer_context_setter(s, ctx);
    }

    constexpr stream_write_result allocate_slot() {
        return stream_write_result::slot_allocated;
    }

    constexpr element_type& get_slot() {
        return buffer;
    }

    constexpr stream_write_result finalize(bool res) {
        if(!streamer.finalize(res)) {
            return stream_write_result::error;
        } else {
            return stream_write_result::value_processed;
        }
    }

    constexpr stream_write_result finalize_item(bool ok) {
        if (!ok) return stream_write_result::error;

        if (!streamer.consume(buffer)) {
            return stream_write_result::error;
        }

        return stream_write_result::value_processed;
    }

    constexpr void reset(){
        streamer.reset();
    }
};

// Map streaming adapters - automatic cursors for map streamers

// Consuming map streamer -> map_write_cursor adapter
template<ConsumingMapStreamerLike Streamer>
struct map_write_cursor<Streamer> {
    using key_type = decltype(std::declval<typename Streamer::value_type>().key);
    using mapped_type = decltype(std::declval<typename Streamer::value_type>().value);
    
    Streamer& streamer;
    typename Streamer::value_type buffer{};
    bool first = true;
    
    constexpr map_write_cursor(Streamer& s)
        : streamer(s)
        , buffer{} {}

    template<class Ctx>
    constexpr map_write_cursor(Streamer& s, Ctx * ctx)
        : streamer(s)
        , buffer{} {
        streamer_context_setter(s, ctx);
    }

    constexpr stream_write_result allocate_key() {
        buffer = typename Streamer::value_type{};
        return stream_write_result::slot_allocated;
    }
    
    constexpr key_type& key_ref() {
        return buffer.key;
    }
    
    constexpr stream_write_result allocate_value_for_parsed_key() {
        return stream_write_result::slot_allocated;
    }
    
    constexpr mapped_type& value_ref() {
        return buffer.value;
    }
    
    constexpr stream_write_result finalize_pair(bool ok) {
        if (!ok) return stream_write_result::error;
        
        if (!streamer.consume(buffer)) {
            return stream_write_result::error;
        } else
            return stream_write_result::value_processed;
    }
    constexpr stream_write_result finalize(bool res) {

        if(!streamer.finalize(res)) {
            return stream_write_result::error;
        } else {
            return stream_write_result::value_processed;
        }
    }

    constexpr void reset() {
        streamer.reset();
    }
};

// Producing map streamer -> map_read_cursor adapter
template<ProducingMapStreamerLike Streamer>
struct map_read_cursor<Streamer> {
    using key_type = decltype(std::declval<typename Streamer::value_type>().key);
    using mapped_type = decltype(std::declval<typename Streamer::value_type>().value);
    
    const Streamer& streamer;
    mutable typename Streamer::value_type buffer{};
    mutable bool first = true;
    
    constexpr map_read_cursor(const Streamer& s)
        : streamer(s)
        , buffer{} {}

    template<class Ctx>
    constexpr map_read_cursor(const Streamer& s, Ctx* ctx)
        : streamer(s)
        , buffer{} {
        streamer_context_setter(s, ctx);
    }

    constexpr stream_read_result read_more() const {
        auto result = streamer.read(buffer);
        if (first) first = false;
        return result;
    }
    
    constexpr const key_type& get_key() const {
        return buffer.key;
    }
    
    constexpr const mapped_type& get_value() const {
        return buffer.value;
    }
    
    constexpr void reset() const {
        streamer.reset();
        first = true;
    }
    constexpr std::size_t size() {
        return -1;
    }
};

namespace detail {
template<class T>
struct always_false : std::false_type {};
}



template<class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template<class T>
concept ParseTransformerLike =
    requires(remove_cvref_t<T> t,
             const typename remove_cvref_t<T>::wire_type& w)
{
    // Must declare wire_type
    typename remove_cvref_t<T>::wire_type;

    // Must implement: bool transform_from(const wire_type&)
    { t.transform_from(w) } -> std::same_as<bool>;
} && ParsableValue<typename remove_cvref_t<T>::wire_type> ;

template<class T, class>
struct parse_transform_traits {
    static constexpr bool is_transformer = false;
    // no wire_type alias here on purpose to avoid hard errors
};

template<class T>
struct parse_transform_traits<
    T,
    std::enable_if_t<ParseTransformerLike<T>>
    >
{
    static constexpr bool is_transformer = true;
    using wire_type = typename remove_cvref_t<T>::wire_type;
};

template<class T>
inline constexpr bool is_parse_transformer_v =
    parse_transform_traits<T>::is_transformer;

template<class T>
concept SerializeTransformerLike =
    requires(const remove_cvref_t<T> t,
             typename remove_cvref_t<T>::wire_type& w)
{
    // Must declare wire_type
    typename remove_cvref_t<T>::wire_type;

    // Must implement: bool transform_to(wire_type&) const
    { t.transform_to(w) } -> std::same_as<bool>;
} && SerializableValue<typename remove_cvref_t<T>::wire_type>;

template<class T, class >
struct serialize_transform_traits {
    static constexpr bool is_transformer = false;
    // no wire_type alias here on purpose to avoid hard errors
};

template<class T>
struct serialize_transform_traits<
    T,
    std::enable_if_t<SerializeTransformerLike<T>>
    >
{
    static constexpr bool is_transformer = true;
    using wire_type = typename remove_cvref_t<T>::wire_type;
};

template<class T>
inline constexpr bool is_serialize_transformer_v =
    serialize_transform_traits<T>::is_transformer;


} //static_schema
namespace schema_analyzis {
using namespace static_schema;
constexpr std::size_t SCHEMA_UNBOUNDED = std::numeric_limits<std::size_t>::max();

template <ParsableValue Type, class ... SeenTypes>
consteval std::size_t calc_type_depth() {
    using T = AnnotatedValue<Type>;

    if constexpr ( (std::is_same_v<T, SeenTypes> || ...) ) {
        return SCHEMA_UNBOUNDED;
    } else {
        // WireSink is a leaf type (depth 1) - doesn't recurse into its members
        if constexpr (WireSinkLike<T>) {
            return 1;
        } else if constexpr(ParseTransformerLike<T>) {
            return  calc_type_depth<typename T::wire_type, SeenTypes...>();
        } else {
            if constexpr (NullableParsableValue<T>) {
                if constexpr (is_specialization_of<T, std::optional>::value) {
                    return calc_type_depth<typename T::value_type, SeenTypes...>();
                } else if constexpr (is_specialization_of<T, std::unique_ptr>::value) {
                    return calc_type_depth<typename T::element_type, SeenTypes...>();
                } else {
                    static_assert(false, "Bad nullable storage");
                    return SCHEMA_UNBOUNDED;
                }
            } else {
                if constexpr (BoolLike<T> || ParsableStringLike<T> || NumberLike<T>){
                    return 1;
                } else { //containers
                    if constexpr(ArrayWritable<T>) {
                        using ElemT = AnnotatedValue<typename array_write_cursor<AnnotatedValue<T>>::element_type>;
                        if(std::size_t r = calc_type_depth<ElemT, T, SeenTypes...>(); r == SCHEMA_UNBOUNDED) {
                            return SCHEMA_UNBOUNDED;
                        } else
                            return 1 + r;
                    } else if constexpr(MapWritable<T>) {
                        using ElemT = AnnotatedValue<typename map_write_cursor<AnnotatedValue<T>>::mapped_type>;
                        if(std::size_t r = calc_type_depth<ElemT, T, SeenTypes...>(); r == SCHEMA_UNBOUNDED) {
                            return SCHEMA_UNBOUNDED;
                        } else
                            return 1 + r;
                    } else { // objects

                        auto fieldDepthGetter = [](auto ic) -> std::size_t {
                            constexpr std::size_t StructIndex = decltype(ic)::value;
                            using Field   = introspection::structureElementTypeByIndex<StructIndex, T>;
                            using Opts    = options::detail::annotation_meta_getter<Field>::options;
                            if constexpr (Opts::template has_option<options::detail::exclude_tag>) {
                                return 0;
                            } else {
                                using FeildT = AnnotatedValue<Field>;
                                return calc_type_depth<FeildT, T, SeenTypes...>();
                            }
                        };
                        constexpr std::size_t struct_elements_count = introspection::structureElementsCount<T>;
                        if constexpr (struct_elements_count == 0) {
                            return 1;
                        } else {
                            std::size_t r = [&]<std::size_t... I>(std::index_sequence<I...>) -> std::size_t {
                                return std::max({fieldDepthGetter(std::integral_constant<std::size_t, I>{})...});
                            }(std::make_index_sequence<struct_elements_count>{});
                            if(r == SCHEMA_UNBOUNDED) {
                                return r;
                            } else {
                                return 1 +r;
                            }
                        }
                    }
                }
            }
        }
    }

}

template <ParsableValue Type, class ... SeenTypes>
consteval std::size_t has_maps() {
    using T = AnnotatedValue<Type>;
    if constexpr ( (std::is_same_v<T, SeenTypes> || ...) ) {
        return false;
    } else {
        // WireSink doesn't contain maps - it's a leaf type
        if constexpr (WireSinkLike<T>) {
            return false;
        } else if constexpr(ParseTransformerLike<T>) {
            return  has_maps<typename T::wire_type, SeenTypes...>();
        } else {
            if constexpr (NullableParsableValue<T>) {
                if constexpr (is_specialization_of<T, std::optional>::value) {
                    return has_maps<typename T::value_type, SeenTypes...>();
                } else if constexpr (is_specialization_of<T, std::unique_ptr>::value) {
                    return has_maps<typename T::element_type, SeenTypes...>();
                } else {
                    static_assert(false, "Bad nullable storage");
                    return SCHEMA_UNBOUNDED;
                }
            } else {
                if constexpr (BoolLike<T> || ParsableStringLike<T> || NumberLike<T>){
                    return false;
                } else { //containers
                    if constexpr(ArrayWritable<T>) {
                        using ElemT = AnnotatedValue<typename array_write_cursor<AnnotatedValue<T>>::element_type>;
                        return has_maps<ElemT, SeenTypes...>();
                    } else if constexpr(MapWritable<T>) {
                        return true;
                    } else { // objects
                        auto mapFinder = [](auto ic) -> std::size_t {
                            constexpr std::size_t StructIndex = decltype(ic)::value;
                            using Field   = introspection::structureElementTypeByIndex<StructIndex, T>;
                            using Opts    = options::detail::annotation_meta_getter<Field>::options;
                            if constexpr (Opts::template has_option<options::detail::exclude_tag>) {
                                return false;
                            } else {
                                using FeildT = AnnotatedValue<Field>;
                                return has_maps<FeildT, T, SeenTypes...>();
                            }
                        };

                        return [&]<std::size_t... I>(std::index_sequence<I...>) -> std::size_t {
                            return (mapFinder(std::integral_constant<std::size_t, I>{}) || ...);
                        }(std::make_index_sequence<introspection::structureElementsCount<T>>{});
                    }
                }
            }
        }
    }
}


}
} // namespace JsonFusion

