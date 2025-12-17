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
#include "struct_introspection.hpp"
#include "static_schema.hpp"
#include "fp_to_str.hpp"


#include "options.hpp"
#include "io.hpp"

namespace JsonFusion {

enum class SerializeError {
    NO_ERROR,
    FIXED_SIZE_CONTAINER_OVERFLOW,
    ILLFORMED_NUMBER,
    STRING_CONTENT_ERROR,
    INPUT_STREAM_ERROR,
    TRANSFORMER_ERROR
};




template <CharOutputIterator OutIter>
class SerializeResult {
    SerializeError m_error = SerializeError::NO_ERROR;
    OutIter m_pos;
public:
    constexpr SerializeResult(SerializeError err, OutIter pos):
        m_error(err), m_pos(pos)
    {}
    constexpr operator bool() const {
        return m_error == SerializeError::NO_ERROR;
    }
    constexpr OutIter pos() const {
        return m_pos;
    }
    constexpr SerializeError error() const {
        return m_error;
    }
};


namespace  serializer_details {


template <CharOutputIterator OutIter>
class SerializationContext {

    SerializeError error = SerializeError::NO_ERROR;
    OutIter m_pos;

public:
    constexpr SerializationContext(OutIter b): m_pos(b) {

    }
    constexpr void setError(SerializeError err, OutIter pos) {
        error = err;
        m_pos = pos;
    }

    constexpr SerializeResult<OutIter> result(OutIter current) const {
        return error == SerializeError::NO_ERROR
                   ? SerializeResult<OutIter>(SerializeError::NO_ERROR, current)
                   : SerializeResult<OutIter>(error, m_pos);
    }
};

constexpr bool serialize_literal(auto& it, const auto& end, const std::string_view & lit) {
    for (char c : lit) {
        if (it == end) {
            return false;
        }
        *it++ = c;
    }
    return true;
}

template <CharOutputIterator It, CharSentinelForOut<It> Sent>
constexpr SerializeError outputEscapedString(It & outputPos, const Sent &end, const char *data, std::size_t size, bool null_ended = false) {
    if(outputPos == end) return SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW;
    *outputPos ++ = '"';
    std::size_t segSize = 0;
    std::size_t counter = 0;
    char toOutEscaped [6] = {0, 0, 0, 0, 0, 0};  // Increased to fit \uXXXX
    char toOutSize = 0;
    bool is_null_end = false;
    while(counter < size) {
        char c = data[counter];
        switch(c) {
        case '"': toOutEscaped[0] = '\\'; toOutEscaped[1] = '"'; toOutSize = 2; break;
        case '\\':toOutEscaped[0] = '\\'; toOutEscaped[1] = '\\';toOutSize = 2; break;
        case '\b':toOutEscaped[0] = '\\'; toOutEscaped[1] = 'b'; toOutSize = 2; break;
        case '\f':toOutEscaped[0] = '\\'; toOutEscaped[1] = 'f'; toOutSize = 2; break;
        case '\r':toOutEscaped[0] = '\\'; toOutEscaped[1] = 'r'; toOutSize = 2; break;
        case '\n':toOutEscaped[0] = '\\'; toOutEscaped[1] = 'n'; toOutSize = 2; break;
        case '\t':toOutEscaped[0] = '\\'; toOutEscaped[1] = 't'; toOutSize = 2; break;
        default:
            if(null_ended && c == 0) {
                is_null_end = true;
            }
            else if(static_cast<unsigned char>(c) < 32) {
                // Control character (0x00-0x1F): escape as \uXXXX per RFC 8259
                toOutEscaped[0] = '\\';
                toOutEscaped[1] = 'u';
                toOutEscaped[2] = '0';
                toOutEscaped[3] = '0';
                unsigned char uc = static_cast<unsigned char>(c);
                toOutEscaped[4] = "0123456789abcdef"[(uc >> 4) & 0xF];
                toOutEscaped[5] = "0123456789abcdef"[uc & 0xF];
                toOutSize = 6;
            } else {
                toOutEscaped[0] = c;
                toOutSize = 1;
            }
        }
        if(is_null_end) {
            break;
        }
        for(int i = 0; i < toOutSize; i++) {
            if(outputPos == end) {
                return SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW;
            }
            *outputPos++ = toOutEscaped[i];
        }
        counter ++;
    }

    if(outputPos == end) return SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW;
    *outputPos ++ = '"';
    return SerializeError::NO_ERROR;
}


template <class Opts, class ObjT, CharOutputIterator It, CharSentinelForOut<It> Sent, class CTX, class UserCtx = void>
    requires static_schema::JsonBool<ObjT>
constexpr bool SerializeNonNullValue(const ObjT & obj, It &currentPos, const Sent & end, CTX &ctx, UserCtx * userCtx = nullptr) {
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
constexpr char* format_decimal_integer(Int value,
                                    char* first,
                                    char* last) noexcept {
    static_assert(std::is_integral_v<Int>, "[[[ JsonFusion ]]] Int must be an integral type");

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
    for (std::size_t i = 0; i < len; ++i)
        first[i] = p[i];
    // std::memmove(first, p, len);
    return first + len;
}

template <class Opts, class ObjT, CharOutputIterator It, CharSentinelForOut<It> Sent, class CTX, class UserCtx = void>
    requires static_schema::JsonNumber<ObjT>
constexpr bool SerializeNonNullValue(const ObjT& obj, It &currentPos, const Sent & end, CTX &ctx, UserCtx * userCtx = nullptr) {
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
            char * endChar = fp_to_str_detail::format_double_to_chars(buf, content, decimals_value);
            auto s = endChar-buf;
            if(endChar-buf == sizeof (buf)) {
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


template <class Opts, class ObjT, CharOutputIterator It, CharSentinelForOut<It> Sent, class CTX, class UserCtx = void>
    requires static_schema::JsonString<ObjT>
constexpr bool SerializeNonNullValue(const ObjT& obj, It &currentPos, const Sent & end, CTX &ctx, UserCtx * userCtx = nullptr) {

    if constexpr(static_schema::DynamicContainerTypeConcept<ObjT>) {
        if(auto err = outputEscapedString(currentPos, end, obj.data(), obj.size(), false); err != SerializeError::NO_ERROR) {
            ctx.setError(err, currentPos);
            return false;
        }
        return true;
    } else {
        if(auto err = outputEscapedString(currentPos, end, static_schema::static_string_traits<ObjT>::data(obj),
                                           static_schema::static_string_traits<ObjT>::max_size(obj), true); err != SerializeError::NO_ERROR) {
            ctx.setError(err, currentPos);
            return false;
        }
        return true;
    }

}

template <class Opts, class ObjT, CharOutputIterator It, CharSentinelForOut<It> Sent, class CTX, class UserCtx = void>
    requires static_schema::JsonSerializableArray<ObjT>
constexpr bool SerializeNonNullValue(const ObjT& obj, It &outputPos, const Sent & end, CTX &ctx, UserCtx * userCtx = nullptr) {
    if(outputPos == end) {
        ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
        return false;
    }
    *outputPos ++ = '[';
    bool first = true;

    using FH   = static_schema::array_read_cursor<ObjT>;
    FH cursor = [&]() {
        if constexpr (!std::is_same_v<UserCtx, void> && std::is_constructible_v<FH, ObjT&, UserCtx*>) {
            if(userCtx) {
                return FH(obj, userCtx);
            } else {
                return FH(obj );
            }
        } else {
            return FH{ obj };
        }
    }();
    cursor.reset();
    while(true) {
        stream_read_result res = cursor.read_more();
        if(res == stream_read_result::end) {
            break;
        } else if(res == stream_read_result::error) {
            ctx.setError(SerializeError::INPUT_STREAM_ERROR, outputPos);
            return false;
        }

        const auto &ch = cursor.get();
        if(first) {
            first = false;
        } else {
            if(outputPos == end) {
                ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
                return false;
            }
            *outputPos ++ = ',';
        }
        using Meta = options::detail::annotation_meta_getter<typename FH::element_type>;
        if(!SerializeValue<typename Meta::options>(Meta::getRef(ch), outputPos, end, ctx, userCtx)) {
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

template <class Opts, class ObjT, CharOutputIterator It, CharSentinelForOut<It> Sent, class CTX, class UserCtx = void>
    requires static_schema::JsonSerializableMap<ObjT>
constexpr bool SerializeNonNullValue(const ObjT& obj, It &outputPos, const Sent & end, CTX &ctx, UserCtx * userCtx = nullptr) {
    if(outputPos == end) {
        ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
        return false;
    }
    *outputPos++ = '{';
    bool first = true;

    using FH = static_schema::map_read_cursor<ObjT>;
    FH cursor = [&]() {
        if constexpr (!std::is_same_v<UserCtx, void> && std::is_constructible_v<FH, ObjT&, UserCtx*>) {
            if(userCtx) {
                return FH(obj, userCtx );
            } else {
                return FH(obj );
            }
        } else {
            return FH{ obj };
        }
    }();
    cursor.reset();
    
    while(true) {
        stream_read_result res = cursor.read_more();
        if(res == stream_read_result::end) {
            break;
        } else if(res == stream_read_result::error) {
            ctx.setError(SerializeError::INPUT_STREAM_ERROR, outputPos);
            return false;
        }
        const auto& key = cursor.get_key();
        const auto& value = cursor.get_value();

        using Meta = options::detail::annotation_meta_getter<typename FH::mapped_type>;
        if constexpr(static_schema::JsonNullableSerializableValue<typename FH::mapped_type> &&
                      Opts::template has_option<options::detail::skip_nulls_tag>) {
            if(static_schema::isNull(value)) {
                continue;
            }
        }

        
        if(first) {
            first = false;
        } else {
            if(outputPos == end) {
                ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
                return false;
            }
            *outputPos++ = ',';
        }
        
        // Serialize key as string
        if constexpr(std::integral<typename FH::key_type>) {
            if(outputPos == end) {
                ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
                return false;
            }
            *outputPos ++ = '"';
        }

        if(!SerializeValue<options::detail::no_options>(key, outputPos, end, ctx, userCtx)) {
            return false;
        }
        if constexpr(std::integral<typename FH::key_type>) {
            if(outputPos == end) {
                ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
                return false;
            }
            *outputPos ++ = '"';
        }

        
        if(outputPos == end) {
            ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
            return false;
        }
        *outputPos++ = ':';
        
        // Serialize value

        if(!SerializeValue<typename Meta::options>(Meta::getRef(value), outputPos, end, ctx, userCtx)) {
            return false;
        }
    }

    if(outputPos == end) {
        ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
        return false;
    }
    *outputPos++ = '}';
    return true;
}


template <bool AsArray, bool indexesAsKeys, bool SkipNulls, std::size_t StructIndex, class ObjT, CharOutputIterator It, CharSentinelForOut<It> Sent, class CTX, class UserCtx>
constexpr bool SerializeOneStructField(std::size_t & count, ObjT& structObj, It &outputPos, const Sent & end, CTX &ctx, UserCtx * userCtx = nullptr) {
    using Field   = introspection::structureElementTypeByIndex<StructIndex, ObjT>;
    using Meta =  options::detail::annotation_meta_getter<Field>;
    // using FieldOpts    = typename Meta::options;
    using FieldOpts = options::detail::aggregate_field_opts_getter<ObjT, StructIndex>;
    if constexpr (FieldOpts::template has_option<options::detail::not_json_tag>) {
        return true;
    } else {
        if constexpr(static_schema::JsonNullableSerializableValue<Field> && SkipNulls){
            if(static_schema::isNull(introspection::getStructElementByIndex<StructIndex>(structObj))) {
                count ++;
                return true;
            }
        }

        if(count != 0) {
            if(outputPos == end) {
                ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
                return false;
            }
            *outputPos ++ = ',';
        }



        if constexpr(!AsArray) {
            if constexpr(indexesAsKeys) {
                if(outputPos == end) {
                    ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
                    return false;
                }
                *outputPos ++ = '"';

                char buf[fp_to_str_detail::NumberBufSize];

                char* p = format_decimal_integer<std::size_t>(count, buf, buf + sizeof(buf));
                for (char* it = buf; it != p; ++it) {
                    if (outputPos == end) {
                        ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
                        return false;
                    }
                    *outputPos++ = *it;
                }

                if(outputPos == end) {
                    ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
                    return false;
                }
                *outputPos ++ = '"';
            } else {
                if constexpr (FieldOpts::template has_option<options::detail::key_tag>) {
                    using KeyOpt = typename FieldOpts::template get_option<options::detail::key_tag>;
                    const auto & f =  KeyOpt::desc.toStringView();
                    if(auto err = outputEscapedString(outputPos, end, f.data(), f.size()); err != SerializeError::NO_ERROR) {
                        ctx.setError(err, outputPos);
                        return false;
                    }
                } else {
                    const auto & f =   introspection::structureElementNameByIndex<StructIndex, ObjT>;
                    if(auto err = outputEscapedString(outputPos, end, f.data(), f.size()); err != SerializeError::NO_ERROR) {
                        ctx.setError(err, outputPos);
                        return false;
                    }
                }
            }

            if(outputPos == end) {
                ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
                return false;
            }
            *outputPos ++ = ':';
        }

        count ++;
        return SerializeValue<FieldOpts>(Meta::getRef(
                                             introspection::getStructElementByIndex<StructIndex>(structObj)
                                             ), outputPos, end, ctx, userCtx);

    }


}
template <bool AsArray, bool indexesAsKeys, bool skipNulls, class ObjT, CharOutputIterator It, CharSentinelForOut<It> Sent, class CTX, class UserCtx, std::size_t... StructIndex>
constexpr bool SerializeStructFields(const ObjT& structObj, It &outputPos, const Sent & end, CTX &ctx, std::index_sequence<StructIndex...>, UserCtx * userCtx = nullptr) {
    std::size_t count = 0;
    return (
        SerializeOneStructField<AsArray, indexesAsKeys, skipNulls, StructIndex>(count, structObj, outputPos, end, ctx, userCtx)
        && ...
        );
}

template <class Opts, class ObjT, CharOutputIterator It, CharSentinelForOut<It> Sent, class CTX, class UserCtx = void>
    requires static_schema::JsonObject<ObjT>
constexpr bool SerializeNonNullValue(const ObjT& obj, It &outputPos, const Sent & end, CTX &ctx, UserCtx * userCtx = nullptr) {

    if(outputPos == end) {
        ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
        return false;
    }
    *outputPos ++ = '{';

    if(!SerializeStructFields<false,
                               Opts::template has_option<options::detail::indexes_as_keys_tag>,
                               Opts::template  has_option<options::detail::skip_nulls_tag>>(obj, outputPos, end, ctx, std::make_index_sequence<introspection::structureElementsCount<ObjT>>{}, userCtx))
        return false;

    if(outputPos == end) {
        ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
        return false;
    }
    *outputPos ++ = '}';
    return true;
}

template <class Opts, class ObjT, CharOutputIterator It, CharSentinelForOut<It> Sent, class CTX, class UserCtx = void>
    requires static_schema::JsonObject<ObjT>
        && Opts::template has_option<options::detail::as_array_tag> // special case for arrays destructuring
constexpr bool SerializeNonNullValue(const ObjT& obj, It &outputPos, const Sent & end, CTX &ctx, UserCtx * userCtx = nullptr) {
    if(outputPos == end) {
        ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
        return false;
    }
    *outputPos ++ = '[';
    bool first = true;



    if(!SerializeStructFields<true, false, false>(obj, outputPos, end, ctx, std::make_index_sequence<introspection::structureElementsCount<ObjT>>{}, userCtx))
        return false;


    if(outputPos == end) {
        ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, outputPos);
        return false;
    }
    *outputPos ++ = ']';
    return true;
}

template <class FieldOptions, static_schema::JsonSerializableValue Field, CharOutputIterator It, CharSentinelForOut<It> Sent, class CTX, class UserCtx = void>
    requires (!static_schema::SerializeTransformer<Field>)
constexpr  bool SerializeValue(const Field & obj, It &currentPos, const Sent & end, CTX &ctx, UserCtx * userCtx = nullptr) {

    if constexpr (FieldOptions::template has_option<options::detail::not_json_tag>) {
        return true;
    } else if constexpr(FieldOptions::template has_option<options::detail::json_sink_tag>) {
        if constexpr(static_schema::DynamicContainerTypeConcept<Field>) {
            for(char ch: obj) {
                if(currentPos == end) {
                    ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, currentPos);
                    return false;
                }
                *currentPos ++ = ch;
            }


        } else {
            char *d = static_schema::static_string_traits<Field>::data(obj);
            for(std::size_t i = 0; i < static_schema::static_string_traits<Field>::max_size(obj); i ++) {
                if(currentPos == end) {
                    ctx.setError(SerializeError::FIXED_SIZE_CONTAINER_OVERFLOW, currentPos);
                    return false;
                }
                *currentPos ++ = d[i];
            }

            return true;
        }
        return true;
    }else if constexpr(static_schema::JsonNullableSerializableValue<Field>) {

        if(static_schema::isNull(obj)) {
            return serialize_literal(currentPos, end, "null");
        }
    }
    return SerializeNonNullValue<FieldOptions>(static_schema::getRef(obj), currentPos, end, ctx, userCtx);
}

template <class FieldOptions, static_schema::JsonSerializableValue Field, CharOutputIterator It, CharSentinelForOut<It> Sent, class CTX, class UserCtx = void>
    requires static_schema::SerializeTransformer<Field>
constexpr  bool SerializeValue(const Field & obj, It &currentPos, const Sent & end, CTX &ctx, UserCtx * userCtx = nullptr) {
    using ObT = static_schema::serialize_transform_traits<Field>::wire_type;
    ObT ob;
    using Meta = options::detail::annotation_meta_getter<ObT>;
    if(!obj.transform_to(ob)) {
        ctx.setError(SerializeError::TRANSFORMER_ERROR, currentPos);
        return false;
    } else {
        return serializer_details::SerializeValue<typename Meta::options>(Meta::getRef(ob), currentPos, end, ctx, userCtx);
    }

}

} // namespace serializer_details




template <static_schema::JsonSerializableValue InputObjectT, CharOutputIterator It, CharSentinelForOut<It> Sent, class UserCtx = void>
constexpr SerializeResult<It> Serialize(const InputObjectT & obj, It &begin, const Sent & end, UserCtx * userCtx = nullptr) {
    serializer_details::SerializationContext<It> ctx(begin);
    using Meta = options::detail::annotation_meta_getter<InputObjectT>;

    serializer_details::SerializeValue<typename Meta::options>(Meta::getRef(obj), begin, end, ctx, userCtx);
    return ctx.result(begin);
}


namespace serializer_details {

}
#if __has_include(<string>)
namespace io_details {
struct limitless_sentinel {};

constexpr bool operator==(const std::back_insert_iterator<std::string>&,
                                 const limitless_sentinel&) noexcept {
    return false;
}

constexpr bool operator==(const limitless_sentinel&,
                                 const std::back_insert_iterator<std::string>&) noexcept {
    return false;
}

constexpr bool operator!=(const std::back_insert_iterator<std::string>& it,
                                 const limitless_sentinel& s) noexcept {
    return !(it == s);
}

constexpr bool operator!=(const limitless_sentinel& s,
                                 const std::back_insert_iterator<std::string>& it) noexcept {
    return !(it == s);
}
}

template<static_schema::JsonSerializableValue InputObjectT, class UserCtx = void>
constexpr auto Serialize(const InputObjectT& obj, std::string& out, UserCtx * ctx = nullptr)
{
    using io_details::limitless_sentinel;

    out.clear();

    auto it  = std::back_inserter(out);
    limitless_sentinel end{};

    return Serialize(obj, it, end, ctx);  // calls the iterator-based core
}
#endif


template <class T>
requires (!static_schema::JsonSerializableValue<T>)
constexpr auto Serialize(T obj, auto C) {
    static_assert(static_schema::detail::always_false<T>::value,
                  "[[[ JsonFusion ]]] T is not a supported JsonFusion serializable value model type.\n"
                  "see JsonSerializableValue concept for full rules");
}


template <class T>
    requires (!static_schema::JsonSerializableValue<T>)
constexpr auto Serialize(T obj, auto C, auto S) {
    static_assert(static_schema::detail::always_false<T>::value,
                  "[[[ JsonFusion ]]] T is not a supported JsonFusion serializable value model type.\n"
                  "see JsonSerializableValue concept for full rules");
}





} // namespace JsonFusion
