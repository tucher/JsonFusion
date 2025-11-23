#pragma once

#include <cstdint>
#include <iterator>
#include <optional>
#include <string_view>
#include <utility>  // std::declval
#include <ranges>
#include <type_traits>
#include <limits>
#include <cmath>
#include <cstring>
#include <pfr.hpp>
#include "static_schema.hpp"
#include "fp_to_str.hpp"


#include "options.hpp"
namespace JsonFusion {

enum class SerializeError {
    NO_ERROR,
    FIXED_SIZE_CONTAINER_OVERFLOW,
    ILLFORMED_NUMBER,
    STRING_CONTENT_ERROR
};


// 1) Iterator you can:
//    - read as *it   (convertible to char)
//    - advance as it++ / ++it
template <class It>
concept CharOutputIterator =
    std::output_iterator<It, char>;

// 2) Matching "end" type you can:
//    - compare as it == end / it != end
template <class It, class Sent>
concept CharSentinelForOut =
    std::sentinel_for<Sent, It>;



template <CharOutputIterator OutIter>
class SerializeResult {
    SerializeError m_error = SerializeError::NO_ERROR;
    OutIter m_pos;
public:
    SerializeResult(SerializeError err, OutIter pos):
        m_error(err), m_pos(pos)
    {}
    operator bool() const {
        return m_error == SerializeError::NO_ERROR;
    }
    OutIter pos() const {
        return m_pos;
    }
    SerializeError error() const {
        return m_error;
    }
};


namespace  serializer_details {


template <CharOutputIterator OutIter>
class SerializationContext {

    SerializeError error = SerializeError::NO_ERROR;
    OutIter m_pos;
public:
    SerializationContext(OutIter b) {
        m_pos = b;
    }
    void setError(SerializeError err, OutIter pos) {
        error = err;
        m_pos = pos;
    }

    SerializeResult<OutIter> result(OutIter current) const {
        return error == SerializeError::NO_ERROR
                   ? SerializeResult<OutIter>(SerializeError::NO_ERROR, current)
                   : SerializeResult<OutIter>(error, m_pos);
    }
};

bool serialize_literal(auto& it, const auto& end, const std::string_view & lit) {
    for (char c : lit) {
        if (it == end) {
            return false;
        }
        *it++ = c;
    }
    return true;
}

template <CharOutputIterator It, CharSentinelForOut<It> Sent>
SerializeError outputEscapedString(It & outputPos, const Sent &end, const char *data, std::size_t size, bool null_ended = false) {
    if(outputPos == end) return SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW;
    *outputPos ++ = '"';
    std::size_t segSize = 0;
    std::size_t counter = 0;
    char toOutEscaped [2] = {0, 0};
    char toOutSize = 0;
    while(counter < size) {
        char c = data[counter];
        switch(c) {
        case '"': toOutEscaped[0] = '\\'; toOutEscaped[1] = '"'; toOutSize = 2; break;
        case '\\':toOutEscaped[0] = '\\'; toOutEscaped[1] = '\\'; toOutSize = 2; break;
        case '\b':toOutEscaped[0] = '\\'; toOutEscaped[1] = 'b'; toOutSize = 2; break;
        case '\f':toOutEscaped[0] = '\\'; toOutEscaped[1] = 'f'; toOutSize = 2; break;
        case '\r':toOutEscaped[0] = '\\'; toOutEscaped[1] = 'r'; toOutSize = 2; break;
        case '\n':toOutEscaped[0] = '\\'; toOutEscaped[1] = 'n'; toOutSize = 2; break;
        case '\t':toOutEscaped[0] = '\\'; toOutEscaped[1] = 't'; toOutSize = 2; break;
        default:
            if(null_ended && c == 0) goto end; //special case for null-terminated;
            else if(c < 32) return SerializeError::STRING_CONTENT_ERROR;
            toOutEscaped[0] = c;
            toOutSize = 1;
        }
        for(int i = 0; i < toOutSize; i++) {
            if(outputPos == end) {
                return SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW;
            }
            *outputPos++ = toOutEscaped[i];
        }
        counter ++;
    }
    end:

    if(outputPos == end) return SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW;
    *outputPos ++ = '"';
    return SerializeError::NO_ERROR;
}


template <class Opts, class ObjT, CharOutputIterator It, CharSentinelForOut<It> Sent>
    requires static_schema::JsonBool<ObjT>
bool SerializeNonNullValue(const ObjT & obj, It &currentPos, const Sent & end, SerializationContext<It> &ctx) {
    if(obj) {
        if(!serialize_literal(currentPos, end, "true")) {
            ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, currentPos);
            return false;
        }
    } else {
        if(!serialize_literal(currentPos, end, "false")) {
            ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, currentPos);
            return false;
        }
    }
    return true;
}

// -------------------------
//  Format decimal integer
// -------------------------
// Writes base-10 representation of value into [first, last).
// Returns pointer one past last written char.
// Caller guarantees buffer is large enough (e.g. NumberBufSize).
template <class Int>
inline char* format_decimal_integer(Int value,
                                    char* first,
                                    char* last) noexcept {
    static_assert(std::is_integral_v<Int>, "Int must be an integral type");

    // We'll generate digits into the end of the buffer, then memmove forward.
    char* p = last;

    using Unsigned = std::make_unsigned_t<Int>;
    Unsigned u;
    bool negative = false;

    if constexpr (std::is_signed_v<Int>) {
        if (value < 0) {
            negative = true;
            // Compute absolute value safely:
            // for min() this avoids UB by doing it in unsigned domain.
            u = Unsigned(-(value + 1)) + 1u;
        } else {
            u = static_cast<Unsigned>(value);
        }
    } else {
        u = static_cast<Unsigned>(value);
    }

    // Generate digits in reverse order
    do {
        if (p == first) {
            // Not enough space; best-effort: just stop
            break;
        }
        unsigned digit = static_cast<unsigned>(u % 10u);
        u /= 10u;
        *--p = static_cast<char>('0' + digit);
    } while (u != 0);

    if (negative) {
        if (p != first) {
            *--p = '-';
        } else {
            // No room for '-', we just overwrite first char
            *p = '-';
        }
    }

    // Now we have the result in [p, last). Move it to start at 'first'.
    std::size_t len = static_cast<std::size_t>(last - p);
    // Assume caller provided a big enough buffer (e.g. NumberBufSize),
    // but clamp just in case.
    if (static_cast<std::size_t>(last - first) < len) {
        len = static_cast<std::size_t>(last - first);
    }

    std::memmove(first, p, len);
    return first + len;
}

template <class Opts, class ObjT, CharOutputIterator It, CharSentinelForOut<It> Sent>
    requires static_schema::JsonNumber<ObjT>
bool SerializeNonNullValue(const ObjT& obj, It &currentPos, const Sent & end, SerializationContext<It> &ctx) {
    char buf[fp_to_str_detail::NumberBufSize];
    if constexpr (std::is_integral_v<ObjT>) {
        char* p = format_decimal_integer<ObjT>(obj, buf, buf + sizeof(buf));
        for (char* it = buf; it != p; ++it) {
            if (currentPos == end) {
                ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, currentPos);
                return false;
            }
            *currentPos++ = *it;
        }
        return true;
    } else {

        double content = obj;

        if(std::isnan(content) || std::isinf(content)) {
            if(!serialize_literal(currentPos, end, "0")) {
                ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, currentPos);
                return false;

            }
            return true;
        } else {
            std::size_t decimals_value = 8;
            if constexpr (Opts::template has_option<options::detail::float_decimals_tag>) {
                using decimals = typename Opts::template get_option<options::detail::float_decimals_tag>;
                decimals_value = decimals::value;
            }
            char * endChar = fp_to_str_detail::format_double_to_chars(buf, buf + sizeof (buf), content, decimals_value);
            auto s = endChar-buf;
            if(endChar-buf == sizeof (buf))  [[unlikely]] {
                return false;
            }

            for(int i = 0; i < s; i ++) {
                if(currentPos == end) {
                    ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, currentPos);
                    return false;
                }
                *currentPos ++ = buf[i];
            }
            return true;

        }
    }
}

template <typename T>
concept DynamicContainerTypeConcept = requires (T  v) {
    typename T::value_type;
    v.push_back(std::declval<typename T::value_type>());
    v.clear();
};

template <class Opts, class ObjT, CharOutputIterator It, CharSentinelForOut<It> Sent>
    requires static_schema::JsonString<ObjT>
bool SerializeNonNullValue(const ObjT& obj, It &currentPos, const Sent & end, SerializationContext<It> &ctx) {

    if(auto err = outputEscapedString(currentPos, end, obj.data(), obj.size(), !DynamicContainerTypeConcept<ObjT>); err != SerializeError::NO_ERROR) {
        ctx.setError(err, currentPos);
        return false;
    }
    return true;
}

template <class Opts, class ObjT, CharOutputIterator It, CharSentinelForOut<It> Sent>
    requires static_schema::JsonArray<ObjT>
bool SerializeNonNullValue(const ObjT& obj, It &outputPos, const Sent & end, SerializationContext<It> &ctx) {
    if(outputPos == end) {
        ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
        return false;
    }
    *outputPos ++ = '[';
    bool first = true;
    for (const auto &ch: obj) {
        if(first) {
            first = false;
        } else {
            if(outputPos == end) {
                ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
                return false;
            }
            *outputPos ++ = ',';
        }
        if(!SerializeValue(ch, outputPos, end, ctx)) {
            return false;
        }
    }

    if(outputPos == end) {
        ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
        return false;
    }
    *outputPos ++ = ']';
    return true;
}

template <class Opts, class ObjT, CharOutputIterator It, CharSentinelForOut<It> Sent>
    requires static_schema::JsonObject<ObjT>
bool SerializeNonNullValue(const ObjT& obj, It &outputPos, const Sent & end, SerializationContext<It> &ctx) {

    if(outputPos == end) {
        ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
        return false;
    }
    *outputPos ++ = '{';

    bool is_first = true;


    bool first = true;
    auto  outputOne = [&outputPos, &end, &ctx, &first,&obj]<std::size_t I>() -> bool {
        const auto & f = pfr::get<I>(obj);
        using Field   = std::remove_cvref_t<decltype(f)>;
        using Meta =  options::detail::annotation_meta_getter<Field>;
        using FieldOpts    = typename Meta::options;


        if constexpr (FieldOpts::template has_option<options::detail::not_json_tag>) {
            return true;
        } else if constexpr (FieldOpts::template has_option<options::detail::not_required_tag> && static_schema::JsonNullableValue<Field>) {
            if (static_schema::isNull(f)) {
                return true;
            }
        }

        if(first) {
            first = false;
        } else {
            if(outputPos == end) {
                ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
                return false;
            }
            *outputPos ++ = ',';
        }
        if constexpr (FieldOpts::template has_option<options::detail::key_tag>) {
            using KeyOpt = typename FieldOpts::template get_option<options::detail::key_tag>;
            const auto & f =  KeyOpt::desc.toStringView();
            if(auto err = outputEscapedString(outputPos, end, f.data(), f.size()); err != SerializeError::NO_ERROR) {
                ctx.setError(err, outputPos);
                return false;
            }
        } else {
            const auto & f =   pfr::get_name<I, ObjT>();
            if(auto err = outputEscapedString(outputPos, end, f.data(), f.size()); err != SerializeError::NO_ERROR) {
                ctx.setError(err, outputPos);
                return false;
            }
        }

        if(outputPos == end) {
            ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
            return false;
        }
        *outputPos ++ = ':';
        return SerializeValue(f, outputPos, end, ctx);
    };

    bool field_parse_result = [&outputOne]<std::size_t... I>(std::index_sequence<I...>) {
        return ( outputOne.template operator()<I>() && ...);

    }(std::make_index_sequence<pfr::tuple_size_v<ObjT>>{});
    if(!field_parse_result) return false;

    if(outputPos == end) {
        ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
        return false;
    }
    *outputPos ++ = '}';
    return true;
}

template <class Opts, class ObjT, CharOutputIterator It, CharSentinelForOut<It> Sent>
    requires static_schema::JsonObject<ObjT>
        && Opts::template has_option<options::detail::as_array_tag> // special case for arrays destructuring
bool SerializeNonNullValue(const ObjT& obj, It &outputPos, const Sent & end, SerializationContext<It> &ctx) {
    if(outputPos == end) {
        ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
        return false;
    }
    *outputPos ++ = '[';
    bool first = true;


    auto try_one = [&](auto ic) {
        constexpr std::size_t StructIndex = decltype(ic)::value;
        using Field   = pfr::tuple_element_t<StructIndex, ObjT>;
        using FieldOpts    = options::detail::annotation_meta_getter<Field>::options;
        if constexpr (FieldOpts::template has_option<options::detail::not_json_tag>) {
            return true;
        } else {
            if(first) {
                first = false;
            } else {
                if(outputPos == end) {
                    ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
                    return false;
                }
                *outputPos ++ = ',';
            }

            if(SerializeValue(pfr::get<StructIndex>(obj), outputPos, end, ctx)) {
                return true;
            } else {
                return false;
            }
        }

    };
    bool ser_result = [&]<std::size_t... I>(std::index_sequence<I...>) {
        return (try_one(std::integral_constant<std::size_t, I>{}) && ...);
    } (std::make_index_sequence<pfr::tuple_size_v<ObjT>>{});


    if(outputPos == end) {
        ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
        return false;
    }
    *outputPos ++ = ']';
    return true;
}

template <static_schema::JsonValue Field, CharOutputIterator It, CharSentinelForOut<It> Sent>
bool SerializeValue(const Field & obj, It &currentPos, const Sent & end, SerializationContext<It> &ctx) {
    using Meta    = options::detail::annotation_meta_getter<Field>;
    if constexpr(static_schema::JsonNullableValue<Field>) {

        if(static_schema::isNull(obj)) {
            return serialize_literal(currentPos, end, "null");
        }
    }
    return SerializeNonNullValue<typename Meta::options>(static_schema::getRef(obj), currentPos, end, ctx);
}

} // namespace serializer_details

template <static_schema::JsonValue InputObjectT, CharOutputIterator It, CharSentinelForOut<It> Sent>
SerializeResult<It> Serialize(const InputObjectT & obj, It begin, const Sent & end) {
    serializer_details::SerializationContext<decltype(begin)> ctx(begin);
    serializer_details::SerializeValue(obj, begin, end, ctx);
    return ctx.result(begin);
}

template<class InputObjectT, class ContainterT>
auto Serialize(const InputObjectT & obj, ContainterT & c) {
    return Serialize(obj, c.begin(), c.end());
}


// Pointer + length front-end
template<static_schema::JsonValue InputObjectT>
SerializeResult<char*> Serialize(const InputObjectT& obj, char* data, std::size_t size) {
    char* begin = data;
    char* end   = data + size;

    serializer_details::SerializationContext<char*> ctx(begin);

    char* cur = begin;
    serializer_details::SerializeValue(obj, cur, end, ctx);

    return ctx.result(begin);
}



} // namespace JsonFusion
