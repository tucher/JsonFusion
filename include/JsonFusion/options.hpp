#pragma once
#include <cstdint>
#include <algorithm>
#include <string_view>
#include <type_traits>
#include <optional>
#include <limits>
#include <utility>
#include <memory>
#include "annotated.hpp"

namespace JsonFusion {

template <typename CharT, std::size_t N> struct ConstString
{
    constexpr bool check()  const {
        for(int i = 0; i < N; i ++) {
            if(std::uint8_t(m_data[i]) < 32) return false;
        }
        return true;
    }
    constexpr ConstString(const CharT (&foo)[N+1]) {
        std::copy_n(foo, N+1, m_data);
    }
    CharT m_data[N+1];
    static constexpr std::size_t Length = N;
    static constexpr std::size_t CharSize = sizeof (CharT);
    constexpr std::string_view toStringView() const {
        return {&m_data[0], &m_data[Length]};
    }
};
template <typename CharT, std::size_t N>
ConstString(const CharT (&str)[N])->ConstString<CharT, N-1>;

template<class Template>
struct is_const_string : std::false_type {};

template<typename CharT, std::size_t N>
struct is_const_string<ConstString<CharT, N>> : std::true_type {};

namespace options {



namespace detail {

struct not_json_tag{};
struct key_tag{};
struct allow_excess_fields_tag{};
struct binary_fields_search_tag{};
struct description_tag{};

struct float_decimals_tag {};
struct as_array_tag {};
struct skip_json_tag {};
struct skip_materializing_tag{};
}

struct not_json {
    using tag = detail::not_json_tag;
};

struct skip_json {
    using tag = detail::skip_json_tag;
};

struct skip_materializing {
    using tag = detail::skip_materializing_tag;
};

template<ConstString Desc>
struct key {
    static_assert(Desc.check(), "[[[ JsonFusion ]]] Jsonkey contains control characters");
    using tag = detail::key_tag;
    static constexpr auto desc = Desc;
};



template<ConstString Desc>
struct description {
    static_assert(Desc.check(), "[[[ JsonFusion ]]] key contains control characters");
    using tag = detail::description_tag;
    static constexpr auto desc = Desc;
};


template<std::size_t N>
struct float_decimals {
    using tag = detail::float_decimals_tag;
    static constexpr std::size_t value = N;
};

struct as_array {
    using tag = detail::as_array_tag;
};


struct allow_excess_fields{
    using tag = detail::allow_excess_fields_tag;
};

struct binary_fields_search{
    using tag = detail::binary_fields_search_tag;
};

namespace detail {


template<class Opt, class Tag, class = void>
struct option_matches_tag : std::false_type {};

template<class Opt, class Tag>
struct option_matches_tag<Opt, Tag, std::void_t<typename Opt::tag>>
    : std::bool_constant<std::is_same_v<typename Opt::tag, Tag>> {};


// === find_option_by_tag ===

template<class Tag, class... Opts>
struct find_option_by_tag;

// Base case: no options → void
template<class Tag>
struct find_option_by_tag<Tag> {
    using type = void;
};

// Recursive case: check First::tag (if present), otherwise continue
template<class Tag, class First, class... Rest>
struct find_option_by_tag<Tag, First, Rest...> {
private:
    using next = typename find_option_by_tag<Tag, Rest...>::type;

public:
    using type = std::conditional_t<
        option_matches_tag<First, Tag>::value,
        First,
        next
        >;
};

struct no_options {
    template<class Tag>
    static constexpr bool has_option = false;

    template<class Tag>
    using get_option = void;
};

template<class T, class... Opts>
struct field_options {

    using underlying_type = T;

    template<class Tag>
    using option_type = typename detail::find_option_by_tag<Tag, Opts...>::type;

    template<class Tag>
    static constexpr bool has_option = !std::is_void_v<option_type<Tag>>;

    template<class Tag>
    using get_option = option_type<Tag>;
};


// Detect Annotated<T, ...>
template<class T>
struct is_annotated : std::false_type {};

template<class U, class... Opts>
struct is_annotated<Annotated<U, Opts...>> : std::true_type {};

template<class T>
inline constexpr bool is_annotated_v = is_annotated<std::remove_cvref_t<T>>::value;



template<class Field>
struct annotation_meta;

// Base: non-annotated, non-optional
template<class T>
struct annotation_meta {
    using value_t = T;
    using options      = no_options;
    static constexpr decltype(auto) getRef(T & f) {
        return (f);
    }
    static constexpr decltype(auto) getRef(const T & f) {
        return (f);
    }
};

// Optional, non-annotated
template<class T, class... Opts>
struct annotation_meta<std::optional<Annotated<T, Opts...>>> {
    static_assert(!sizeof(T), "[[[ JsonFusion ]]] Use Annotated<std::optional<T>, ...> instead of std::optional<Annotated<T, ...>>");
};

template<class T, class... Opts>
struct annotation_meta<std::unique_ptr<Annotated<T, Opts...>>> {
    static_assert(!sizeof(T), "[[[ JsonFusion ]]] Use Annotated<std::unique_ptr<T>, ...> instead of std::unique_ptr<Annotated<T, ...>>");
};

// Annotated<T, Opts...>
template<class T, class... Opts>
struct annotation_meta<Annotated<T, Opts...>> {
    // static_assert((requires { typename Opts::tag; } && ...));
    using value_t = T;
    using options      = field_options<T, Opts...>;

    static constexpr decltype(auto) getRef(Annotated<T, Opts...> & f) {
        return (f.value);
    }
    static constexpr decltype(auto) getRef(const Annotated<T, Opts...> & f) {
        return (f.value);
    }
};


// Entry point with decay
template<class Field>
struct annotation_meta_getter : annotation_meta<std::remove_cvref_t<Field>> {};



} // namespace detail


} //namespace options


} // namespace JsonFusion
