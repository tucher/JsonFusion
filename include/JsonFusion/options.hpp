#pragma once
#include <cstdint>
#include <algorithm>
#include <string_view>
#include <type_traits>
#include <optional>

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



namespace options {

namespace detail {
struct not_json_tag{};
struct key_tag{};
struct not_required_tag{};
struct allow_excess_fields_tag{};
struct range_tag{};
struct description_tag{};
struct min_length_tag {};
struct max_length_tag {};
struct min_items_tag {};
struct max_items_tag {};
struct float_decimals_tag {};
struct as_array_tag {};

}

struct not_json {
    using tag = detail::not_json_tag;
};


template<ConstString Desc>
struct key {
    static_assert(Desc.check(), "key contains control characters");
    using tag = detail::key_tag;
    static constexpr auto desc = Desc;
};

struct not_required {
    using tag = detail::not_required_tag;
};

struct allow_excess_fields {
    using tag = detail::allow_excess_fields_tag;
};

template<auto Min, auto Max>
struct range {
    using tag = detail::range_tag;
    static constexpr auto min = Min;
    static constexpr auto max = Max;
};

template<ConstString Desc>
struct description {
    static_assert(Desc.check(), "key contains control characters");
    using tag = detail::description_tag;
    static constexpr auto desc = Desc;
};

template<std::size_t N>
struct min_length {
    using tag = detail::min_length_tag;
    static constexpr std::size_t value = N;
};

template<std::size_t N>
struct max_length {
    using tag = detail::max_length_tag;
    static constexpr std::size_t value = N;
};

template<std::size_t N>
struct min_items {
    using tag = detail::min_items_tag;
    static constexpr std::size_t value = N;
};

template<std::size_t N>
struct max_items {
    using tag = detail::max_items_tag;
    static constexpr std::size_t value = N;
};

template<std::size_t N>
struct float_decimals {
    using tag = detail::float_decimals_tag;
    static constexpr std::size_t value = N;
};

struct as_array {
    using tag = detail::as_array_tag;
};

namespace detail {


template<class Tag, class... Opts>
struct find_option_by_tag;

// Base case: no options → void
template<class Tag>
struct find_option_by_tag<Tag> {
    using type = void;
};

// Recursive case: check First::tag, or continue
template<class Tag, class First, class... Rest>
struct find_option_by_tag<Tag, First, Rest...> {
private:
    using first_tag = std::conditional_t<
        requires { typename First::tag; },
        typename First::tag,
        void
        >;

    using next = typename find_option_by_tag<Tag, Rest...>::type;

public:
    using type = std::conditional_t<
        std::is_same_v<Tag, first_tag>,
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
    static decltype(auto) getRef(T & f) {
        return (f);
    }
    static decltype(auto) getRef(const T & f) {
        return (f);
    }
};

// Optional, non-annotated
template<class T, class... Opts>
struct annotation_meta<std::optional<Annotated<T, Opts...>>> {
    static_assert(!sizeof(T), "Use Annotated<std::optional<T>, ...> instead of std::optional<Annotated<T, ...>>");
};

// Annotated<T, Opts...>
template<class T, class... Opts>
struct annotation_meta<Annotated<T, Opts...>> {
    using value_t = T;
    using options      = field_options<T, Opts...>;

    static decltype(auto) getRef(Annotated<T, Opts...> & f) {
        return (f.value);
    }
    static decltype(auto) getRef(const Annotated<T, Opts...> & f) {
        return (f.value);
    }
};


// Entry point with decay
template<class Field>
struct annotation_meta_getter : annotation_meta<std::remove_cvref_t<Field>> {};



} // namespace detail


} //namespace options


} // namespace JsonFusion
