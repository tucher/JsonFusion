#pragma once

#include <array>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string_view>
#include <utility>  // std::declval
#include <ranges>
#include <type_traits>
#include <pfr.hpp>
#include "static_schema.hpp"
#include <fast_double_parser.h>

namespace JSONReflection2 {

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

template<ConstString Desc>
struct key {
    static constexpr auto desc = Desc;
};

struct required {};

template<auto Min, auto Max>
struct range {
    static constexpr auto min = Min;
    static constexpr auto max = Max;
};

template<ConstString Desc>
struct description {
    static constexpr auto desc = Desc;
};

template<typename... Opts>
inline constexpr bool is_required_v =
    (std::is_same_v<std::remove_cv_t<Opts>, required> || ...);

// Generic "find specialization of Template in a pack"
template<template<auto...> class Template, typename... Opts>
struct find_specialization;

// Base case: not found
template<template<auto...> class Template>
struct find_specialization<Template> {
    using type = void;   // sentinel for "not found"
};

// Skip non-matching head
template<template<auto...> class Template, typename Head, typename... Tail>
struct find_specialization<Template, Head, Tail...>
    : find_specialization<Template, Tail...> {};

// Match Template<Args...>
template<template<auto...> class Template, auto... Args, typename... Tail>
struct find_specialization<Template, Template<Args...>, Tail...> {
    using type = Template<Args...>;
};

template<template<auto...> class Template, typename... Opts>
using find_specialization_t = typename find_specialization<Template, Opts...>::type;


namespace detail {

// pack_contains<T, Ts...>
template<class T, class... Ts>
struct pack_contains : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

template<class T, class... Ts>
inline constexpr bool pack_contains_v = pack_contains<T, Ts...>::value;

// Find range_t<Min, Max> in an options pack

template<class... Opts>
struct range_finder {
    static constexpr bool has_range = false;
    // min/max are intentionally *not* defined here when has_range == false
};

template<class First, class... Rest>
struct range_finder<First, Rest...> : range_finder<Rest...> {};

// specialization when we hit a range_t
template<auto Min, auto Max, class... Rest>
struct range_finder<range<Min, Max>, Rest...> {
    static constexpr bool has_range = true;
    static constexpr auto min = Min;
    static constexpr auto max = Max;
};

} // namespace detail

struct no_options {
    static constexpr bool is_required = false;
    static constexpr bool has_range   = false;
};


template<typename T, typename... Opts>
struct field_options {
    static constexpr bool is_required =
        detail::pack_contains_v<required, Opts...>;

    using range_info = detail::range_finder<Opts...>;

    static constexpr bool has_range = range_info::has_range;

    // Only valid if has_range == true
    static constexpr auto min = []{
        if constexpr (range_info::has_range) return range_info::min;
        else return T{}; // never used if !has_range
    }();

    static constexpr auto max = []{
        if constexpr (range_info::has_range) return range_info::max;
        else return T{};
    }();
};



}




template <class T, typename... Options>
struct Annotated {
    using value_type   = T;
    using options      = options::field_options<T, Options...>; //more likely will be used in parser, not here

    T value{};

    // Access
    T&       get()       { return value; }
    const T& get() const { return value; }

    // Conversions/forwarding so parser can treat Annotated<T> like T
    operator T&()             { return value; }
    operator const T&() const { return value; }

    Annotated& operator=(const T& v) {
        value = v;
        return *this;
    }
};

enum ErrorT {
    NO_ERROR,
    UNEXPECTED_END_OF_DATA,
    UNEXPECTED_SYMBOL,
    FIXED_SIZE_CONTAINER_OVERFLOW,
    ILLFORMED_NUMBER,
    ILLFORMED_BOOL,
    EXCESS_FIELD,
    NULL_IN_NON_OPTIONAL,
    ILLFORMED_NULL,
    ILLFORMED_STRING,
    ILLFORMED_ARRAY,
    ILLFORMED_OBJECT,
    EXCESS_DATA
};


// 1) Iterator you can:
//    - read as *it   (convertible to char)
//    - advance as it++ / ++it
template <class It>
concept CharInputIterator =
    std::input_iterator<It> &&
    std::convertible_to<std::iter_reference_t<It>, char>;

// 2) Matching "end" type you can:
//    - compare as it == end / it != end
template <class It, class Sent>
concept CharSentinelFor =
    CharInputIterator<It> &&
    std::sentinel_for<Sent, It>;



template <CharInputIterator InpIter>
class ParseResult {
    ErrorT m_error = NO_ERROR;
    InpIter m_pos;
public:
    ParseResult(ErrorT err, InpIter pos):
        m_error(err), m_pos(pos)
    {}
    operator bool() const {
        return m_error == NO_ERROR;
    }
    InpIter pos() const {
        return m_pos;
    }
    ErrorT error() const {
        return m_error;
    }
};

enum class ParseFlags: std::uint32_t {
    DEFAULT = 0,
    EXCESS_FIELDS_PROHIBITED = std::underlying_type_t<ParseFlags>(1) << 1,
    ALL_FIELDS_REQUIRED =      std::underlying_type_t<ParseFlags>(1) << 2,
};

inline constexpr ParseFlags operator| (const ParseFlags &l, const ParseFlags &r) {
    return ParseFlags(static_cast<std::underlying_type_t<ParseFlags>>(l) | static_cast<std::underlying_type_t<ParseFlags>>(r));
}

namespace  parser_details {




template<class Field>
struct field_meta {
    using storage_type = std::remove_cvref_t<Field>;
    using options      = options::no_options;
};

// Annotated<T, Options...> specialization
template<class T, class... Opts>
struct field_meta<Annotated<T, Opts...>> {
    using storage_type = T;
    using options      = options::field_options<T, Opts...>;
};

// optional<...> – just strip optional, options come from inside
template<class U>
struct field_meta<std::optional<U>> : field_meta<U> {};

template<class ObjT>
decltype(auto) get_storage(ObjT& obj) {
    return (obj);
}

template<class T, class... Opts>
decltype(auto) get_storage(Annotated<T, Opts...>& a) {
    return (a.value);
}

template<class T>
decltype(auto) get_storage(std::optional<T>& o) {
    if (!o) o.emplace();
    return *o;
}

template<class T, class... Opts>
decltype(auto) get_storage(std::optional<Annotated<T, Opts...>>& o) {
    if (!o) o.emplace();
    return o->value;
}

template <CharInputIterator InpIter>
class DeserializationContext {

    ErrorT error = NO_ERROR;
    InpIter m_pos;
public:
    DeserializationContext(InpIter b) {
        m_pos = b;
    }
    void setError(ErrorT err, InpIter pos) {
        error = err;
        m_pos = pos;
    }

    ParseResult<InpIter> result() {
        return ParseResult<InpIter>(error, m_pos);
    }
};

inline bool isSpace(char a) {
    switch(a) {
    case 0x20:
    case 0x0A:
    case 0x0D:
    case 0x09:
        return true;
    }
    return false;
}

inline bool isPlainEnd(char a) {
    switch(a) {
    case ']':
    case ',':
    case '}':
    case 0x20:
    case 0x0A:
    case 0x0D:
    case 0x09:
        return true;
    }
    return false;
}

template <CharInputIterator It, CharSentinelFor<It> Sent>
inline bool skipWhiteSpace(It & currentPos, const Sent & end, DeserializationContext<It> & ctx) {
    while (currentPos != end && isSpace(*currentPos)) {
        ++currentPos;
    }
    if (currentPos == end) {
        ctx.setError(ErrorT::UNEXPECTED_END_OF_DATA, currentPos);
        return false;
    }
    return true;
}

template <class T>
struct decay_optional {
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

// specialization for std::optional
template <class U>
struct decay_optional<std::optional<U>> {
    using type = std::remove_cv_t<std::remove_reference_t<U>>;
};

template <class T>
using decay_optional_t = typename decay_optional<T>::type;

bool match_literal(auto& it, const auto& end, std::string_view lit) {
    for (char c : lit) {
        if (it == end || *it != c) {
            return false;
        }
        ++it;
    }
    return true;
}

template <ParseFlags flags, class ObjT, CharInputIterator It, CharSentinelFor<It> Sent>
    requires static_schema::JsonBool<decay_optional_t<ObjT>>
bool ParseNonNullValue(ObjT & obj, It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {
    auto& storage = get_storage(obj);
    if(currentPos == end) {
        ctx.setError(ErrorT::UNEXPECTED_END_OF_DATA, currentPos);
        return false;
    }
    if (*currentPos == 't' && match_literal(++currentPos, end, "rue")) {
        storage = true;
        return true;
    } else if (*currentPos == 'f' && match_literal(++currentPos, end, "alse")) {
        storage = false;
        return true;
    } else {
        ctx.setError(ErrorT::ILLFORMED_BOOL, currentPos);
        return false;
    }
}



template <ParseFlags flags, class ObjT, CharInputIterator It, CharSentinelFor<It> Sent>
    requires static_schema::JsonNumber<decay_optional_t<ObjT>>
bool ParseNonNullValue(ObjT& obj, It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {
    auto& storage = get_storage(obj);
    using Opts    = field_meta<parser_details::decay_optional_t<ObjT>>::options;
    constexpr std::size_t NumberBufSize = 40;
    char buf[NumberBufSize];
    std::size_t index = 0;
    while(currentPos != end && !isPlainEnd(*currentPos)) {
        buf[index] = *currentPos;
        currentPos ++;
        index ++;
        if(index > NumberBufSize - 1) [[unlikely]] {
            ctx.setError(ErrorT::ILLFORMED_NUMBER, currentPos);
            return false;
        }
    }
    buf[index] = 0;
    double x;
    if(fast_double_parser::parse_number(buf, &x) != nullptr) {
        //TODO checks and options
        if constexpr (Opts::has_range) {
            if (x < Opts::min || x > Opts::max) {
                ctx.setError(ErrorT::ILLFORMED_NUMBER, currentPos);
                return false;
            }
        }

        storage = x;
        return true;
    } else {
        ctx.setError(ErrorT::ILLFORMED_NUMBER, currentPos);
        return false;
    }
}

template <typename T>
concept DynamicContainerTypeConcept = requires (T  v) {
    typename T::value_type;
    v.push_back(std::declval<typename T::value_type>());
    v.clear();
};

template <class Visitor, CharInputIterator It, CharSentinelFor<It> Sent>
bool parseString(Visitor&& inserter, It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {
    if(*currentPos != '"'){
        ctx.setError(ErrorT::ILLFORMED_STRING, currentPos);
        return false;
    }
    currentPos++;

    while(true) {
        if(currentPos == end) {
            ctx.setError(ErrorT::UNEXPECTED_END_OF_DATA, currentPos);
            return false;
        }
        auto c = *currentPos;
        switch(c) {
        case '"': {
            currentPos ++;

            return true;
        }
        case '\\': {
            currentPos++;
            if(currentPos == end) [[unlikely]] {
                ctx.setError(ErrorT::UNEXPECTED_END_OF_DATA, currentPos);
                return false;
            }


            switch (*currentPos) {
            /* Allowed escaped symbols */
            case '\"':
            case '/':
            case '\\':
            case 'b':
            case 'f':
            case 'r':
            case 'n':
            case 't':
            {
                char unescaped[2] = {0, 0};
                switch (*currentPos) {
                case '"':  unescaped[0] = '"'; break;
                case '/':  unescaped[0] = '/'; break;
                case '\\': unescaped[0] = '\\'; break;
                case 'b':  unescaped[0] = '\b'; break;
                case 'f':  unescaped[0] = '\f'; break;
                case 'r':  unescaped[0] = '\r'; break;
                case 'n':  unescaped[0] = '\n'; break;
                case 't':  unescaped[0] = '\t'; break;
                }

                if(!inserter(unescaped[0])) {
                    ctx.setError(ErrorT::FIXED_SIZE_CONTAINER_OVERFLOW, currentPos);
                    return false;
                }
                currentPos++;
            }
            break;
                /* Allows escaped symbol \uXXXX */
            case 'u': {

                currentPos++;
                std::array<char, 4> utf8bytes;
                auto utfI = utf8bytes.begin();
                while(true) {
                    if(currentPos == end) [[unlikely]] {
                        ctx.setError(ErrorT::UNEXPECTED_END_OF_DATA, currentPos);
                        return false;
                    }
                    if(utfI ==  utf8bytes.end())[[unlikely]] {
                        break;
                    }
                    char currChar = *currentPos;
                    if (currChar >= 48 && currChar <= 57) {     /* 0-9 */
                        *utfI = currChar-'0';
                    } else if(currChar >= 65 && currChar <= 70){     /* A-F */
                        *utfI = currChar-'A' + 10;
                    } else if(currChar >= 97 && currChar <= 102) { /* a-f */
                        *utfI = currChar-'a' + 10;
                    }
                    else {
                        ctx.setError(ErrorT::UNEXPECTED_SYMBOL, currentPos);
                        return false;
                    }

                    currentPos++;
                    utfI ++;
                }
                if(currentPos == end) [[unlikely]] {
                    ctx.setError(ErrorT::UNEXPECTED_END_OF_DATA, currentPos);
                    return false;
                }
                char unescaped[3] = {0, 0, 0};
                unescaped[0] |= (utf8bytes[0] << 4);
                unescaped[0] |= utf8bytes[1];
                unescaped[1] |= (utf8bytes[2] << 4);
                unescaped[1] |= utf8bytes[3];
                if(!inserter(unescaped[0]) || !inserter(unescaped[1])) {
                    ctx.setError(ErrorT::FIXED_SIZE_CONTAINER_OVERFLOW, currentPos);
                    return false;
                }

            }
            break;
                /* Unexpected symbol */
            default:
                ctx.setError(ErrorT::UNEXPECTED_SYMBOL, currentPos);
                return false;
            }

            break;
        }

        default:
            if(!inserter(*currentPos)) {
                ctx.setError(ErrorT::FIXED_SIZE_CONTAINER_OVERFLOW, currentPos);
                return false;
            }
            currentPos++;
        }
    }
}


template <ParseFlags flags, class ObjT, CharInputIterator It, CharSentinelFor<It> Sent>
    requires static_schema::JsonString<decay_optional_t<ObjT>>
bool ParseNonNullValue(ObjT& obj, It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {
    using T = static_schema::JsonUnderlying<decay_optional_t<ObjT>>;

    auto& storage = get_storage(obj);
    std::size_t fixedSizeContainerPos = 0;
    if constexpr (DynamicContainerTypeConcept<T>) {
        storage.clear();
    }
    auto inserter = [&storage, &fixedSizeContainerPos] (char c) -> bool {
        if constexpr (!DynamicContainerTypeConcept<T>) {
            if (fixedSizeContainerPos < storage.size()) {
                storage[fixedSizeContainerPos] = c;
                fixedSizeContainerPos ++;
                return true;
            } else {
               return false;
            }
        }  else {
            storage.push_back(c);
            return true;
        }
    };

    if(parseString(inserter, currentPos, end, ctx)) {
        if constexpr (!DynamicContainerTypeConcept<T>) {
            if(fixedSizeContainerPos < storage.size())
                storage[fixedSizeContainerPos] = 0;
        }
        return true;
    } else {
        return false;
    }
}

template <ParseFlags flags, class ObjT, CharInputIterator It, CharSentinelFor<It> Sent>
    requires static_schema::JsonArray<decay_optional_t<ObjT>>
bool ParseNonNullValue(ObjT& obj, It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {
    using T = static_schema::JsonUnderlying<decay_optional_t<ObjT>>;
    auto& storage = get_storage(obj);
    if(*currentPos != '[') {
        ctx.setError(ErrorT::ILLFORMED_ARRAY, currentPos);
        return false;
    }
    currentPos++;
    if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
        return false;
    }
    std::size_t static_container_index = 0;
    bool has_traling_comma = false;
    while(true) {
        if(currentPos == end) [[unlikely]] {
            ctx.setError(ErrorT::UNEXPECTED_END_OF_DATA, currentPos);
            return false;
        }
        if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
            return false;
        }
        if(*currentPos == ']') {
            if(has_traling_comma) {
                ctx.setError(ErrorT::ILLFORMED_ARRAY, currentPos);
                return false;
            }
            currentPos ++;
            return true;
        }
        if constexpr (DynamicContainerTypeConcept<T>) {
            auto & newItem = storage.emplace_back();
            if(!ParseValue<flags>(newItem, currentPos, end, ctx)) {
                return false;
            }
        } else {
            if(static_container_index < storage.size()) {
                if(!ParseValue<flags>(storage[static_container_index], currentPos, end, ctx)) {
                    return false;
                }
                static_container_index ++;
            } else {
                ctx.setError(ErrorT::FIXED_SIZE_CONTAINER_OVERFLOW, currentPos);
                return false;
            }
            //TODO handle different size policies via opts
        }
        if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
            return false;
        }
        has_traling_comma = false;
        if(*currentPos == ',') {
            has_traling_comma = true;
            currentPos ++;
        }

    }
    return false;
}

struct FieldDescr {
    std::string_view name;
    std::size_t originalIndex;
};

struct IncrementalFieldSearch {
    using It = const FieldDescr*;

    It first;         // current candidate range [first, last)
    It last;
    It original_end;  // original end, used as "empty result"
    std::size_t depth = 0; // how many characters have been fed

    IncrementalFieldSearch(It begin, It end)
        : first(begin), last(end), original_end(end), depth(0) {}

    // Feed next character; narrows [first, last) by character at position `depth`.
    // Returns true if any candidates remain after this step.
    bool step(char ch) {
        if (first == last)
            return false;

        // projection: FieldDescr -> char at current depth
        auto char_at_depth = [this](const FieldDescr& f) -> char {
            std::string_view s = f.name;
            // sentinel: '\0' means "past the end"
            return depth < s.size() ? s[depth] : '\0';
        };

        // lower_bound: first element with char_at_depth(elem) >= ch
        auto lower = std::ranges::lower_bound(
            first, last, ch, {}, char_at_depth
            );

        // upper_bound: first element with char_at_depth(elem) > ch
        auto upper = std::ranges::upper_bound(
            lower, last, ch, {}, char_at_depth
            );

        first = lower;
        last  = upper;
        ++depth;
        return first != last;
    }

    // Return:
    //   - pointer to unique FieldDescr if exactly one matches AND
    //     fully typed (depth >= name.size())
    //   - original_end if 0 matches OR >1 matches OR undertyped.
    It result() const {
        if (first == last)
            return original_end; // 0 matches

        It it = first;
        ++it;
        if (it != last)
            return original_end; // >1 match => ambiguous

        // exactly one candidate
        const FieldDescr& candidate = *first;

        // undertyping: not all characters of the name have been given
        if (depth < candidate.name.size())
            return original_end;

        return first;
    }
};

template<class T>
struct FieldsHelper {
    static constexpr std::size_t fieldsCount = pfr::tuple_size_v<T>;

    static constexpr std::array<FieldDescr, fieldsCount> fieldIndexesSortedByFieldName = []<std::size_t... I>(std::index_sequence<I...>) {
        //TODO get name from options, if presented
        std::array<FieldDescr, fieldsCount> arr = {FieldDescr{pfr::get_name<I, T>(), I} ...};
        std::ranges::sort(arr, {}, &FieldDescr::name);
        return arr;
    }(std::make_index_sequence<fieldsCount>{});
};

template <ParseFlags flags, class ObjT, CharInputIterator It, CharSentinelFor<It> Sent>
    requires static_schema::JsonObject<decay_optional_t<ObjT>>
bool ParseNonNullValue(ObjT& obj, It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {
    using T = static_schema::JsonUnderlying<decay_optional_t<ObjT>>;
    auto& storage = get_storage(obj);

    if(*currentPos != '{') {
        ctx.setError(ErrorT::ILLFORMED_OBJECT, currentPos);
        return false;
    }
    currentPos++;
    if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
        return false;
    }

    while(true) {
        if(currentPos == end) [[unlikely]] {
            ctx.setError(ErrorT::UNEXPECTED_END_OF_DATA, currentPos);
            return false;
        }
        if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
            return false;
        }

        if(*currentPos == '}') {
            currentPos ++;

            //TODO Do final checks here for required or excess fields
            return true;
        }

        IncrementalFieldSearch searcher{
            FieldsHelper<T>::fieldIndexesSortedByFieldName.data(), FieldsHelper<T>::fieldIndexesSortedByFieldName.data() + FieldsHelper<T>::fieldIndexesSortedByFieldName.size()
        };

        if(!parseString([&searcher](char c){
                searcher.step(c);
                return true;
            }, currentPos, end, ctx)) {
            return false;
        }
        auto res = searcher.result();
        if(res == FieldsHelper<T>::fieldIndexesSortedByFieldName.end()) {
            //TODO handle excess fields
            ctx.setError(ErrorT::EXCESS_FIELD, currentPos);
            return false;
        }

        if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
            ctx.setError(ErrorT::ILLFORMED_OBJECT, currentPos);
            return false;
        }
        if(*currentPos != ':') {
            ctx.setError(ErrorT::ILLFORMED_OBJECT, currentPos);
            return false;
        }
        currentPos ++;
        if(currentPos == end) [[unlikely]] {
            ctx.setError(ErrorT::UNEXPECTED_END_OF_DATA, currentPos);
            return false;
        }


        bool field_parse_result = [&res, &currentPos, &end, &ctx, &storage]<std::size_t... I>(std::index_sequence<I...>) {
            return (
                (I==res->originalIndex && ParseValue<flags>(pfr::get<I>(storage), currentPos, end, ctx)) || ...);

        }(std::make_index_sequence<pfr::tuple_size_v<T>>{});

        if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
            return false;
        }
        if(*currentPos == ',') {
            currentPos ++;
        }

    }
    return false;
}



template <ParseFlags flags, CharInputIterator It, CharSentinelFor<It> Sent>
bool ParseValue(static_schema::JsonValue auto & obj, It &currentPos, const Sent & end, DeserializationContext<It> &ctx) {

    if(!skipWhiteSpace(currentPos, end, ctx)) [[unlikely]] {
        return false;
    }
    if constexpr(static_schema::JsonNullableValue<decltype(obj)>) {
        if (currentPos == end) {
            ctx.setError(ErrorT::UNEXPECTED_END_OF_DATA, currentPos);
            return false;
        }
        if(*currentPos == 'n') {
            currentPos++;
            if(match_literal(currentPos, end, "ull")) {
                obj.reset();
                return true;
            } else {
                ctx.setError(ErrorT::ILLFORMED_NULL, currentPos);
                return false;
            }
        }
    } else {
        if (currentPos != end && *currentPos == 'n') {
            ctx.setError(ErrorT::NULL_IN_NON_OPTIONAL, currentPos);
            return false;
        }
    }
    return ParseNonNullValue<flags>(obj, currentPos, end, ctx);
}

} // namespace parser_details

template <ParseFlags flags=ParseFlags::DEFAULT, static_schema::JsonValue InputObjectT, CharInputIterator It, CharSentinelFor<It> Sent>
ParseResult<It> Parse(InputObjectT & obj, It begin, const Sent & end) {
    InputObjectT copy = obj;
    parser_details::DeserializationContext<decltype(begin)> ctx(begin);


    parser_details::ParseValue<flags>(obj, begin, end, ctx);

    auto res = ctx.result();
    if(!res) {
        obj = copy;
    } else {
        if(parser_details::skipWhiteSpace(begin, end, ctx)) {
            ctx.setError(ErrorT::EXCESS_DATA, begin);
        } else {
            ctx.setError(ErrorT::NO_ERROR, begin);
        }
    }
    return res;
}

template<ParseFlags flags=ParseFlags::DEFAULT, class InputObjectT, class ContainterT>
auto Parse(InputObjectT & obj, const ContainterT & c) {
    return Parse<flags>(obj, c.begin(), c.end());
}


} // namespace JSONReflection2
