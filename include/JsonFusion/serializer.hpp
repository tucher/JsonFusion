#pragma once

#include <cstdint>
#include <iterator>
#include <optional>
#include <string_view>
#include <utility>  // std::declval
#include <ranges>
#include <type_traits>
#include <limits>
#include "struct_introspection.hpp"
#include "static_schema.hpp"
#include "struct_fields_helper.hpp"
#include "validators.hpp"


#include "options.hpp"
#include "io.hpp"

#include "json.hpp"
#include "wire_sink.hpp"

namespace JsonFusion {

enum class SerializeError {
    NO_ERROR,
    INPUT_STREAM_ERROR,
    TRANSFORMER_ERROR,
    WRITER_ERROR,
    SCHEMA_VALIDATION_ERROR
};

template <class OutIter, class WriterError>
class SerializeResult {
    SerializeError m_error = SerializeError::NO_ERROR;
    WriterError m_writerError{};
    OutIter m_pos;
    ValidationResult validationResult;
    std::size_t m_written = -1;
public:
    constexpr SerializeResult(SerializeError err, WriterError werr, ValidationResult schemaErrors, OutIter pos, std::size_t wr):
        m_error(err), m_writerError(werr), m_pos(pos), validationResult(schemaErrors), m_written(wr)
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
    constexpr WriterError writerError() const {
        return m_writerError;
    }
    constexpr std::size_t bytesWritten() const {
        return m_written;
    }
};


namespace  serializer_details {


template <class OutIter, class WriterError>
class SerializationContext {

    SerializeError error = SerializeError::NO_ERROR;
    WriterError writerError{};
    OutIter m_pos;
    validators::validators_detail::ValidationCtx _validationCtx;
    std::size_t m_written = -1;
public:
    constexpr SerializationContext(OutIter it): m_pos(it){}
    template<class Writer>
    constexpr bool withWriterError(Writer & writer) {
        error = SerializeError::WRITER_ERROR;
        writerError = writer.getError();
        m_pos = writer.current();
        return false;
    }

    template<class Writer>
    constexpr bool withError(SerializeError err, Writer & writer) {
        if(err == SerializeError::NO_ERROR) {
            error = SerializeError::WRITER_ERROR;
        }
        writerError = writer.getError();
        m_pos = writer.current();
        return false;
    }
    constexpr SerializeError currentError(){return error;}

    constexpr SerializeResult<OutIter, WriterError> result() const {
        return SerializeResult<OutIter, WriterError>(error, writerError, _validationCtx.result(), m_pos, m_written);
    }

    constexpr validators::validators_detail::ValidationCtx & validationCtx() {return _validationCtx;}
    constexpr bool withSchemaError(writer::WriterLike auto & writer) {
        error = SerializeError::SCHEMA_VALIDATION_ERROR;
        m_pos = writer.current();
        return false;
    }
    constexpr void setBytesWritten(std::size_t written) {
        m_written = written;
    }


};


template <class Opts, class ObjT, writer::WriterLike Writer, class CTX, class UserCtx = void>
    requires static_schema::BoolLike<ObjT>
constexpr bool SerializeNonNullValue(const ObjT & obj, Writer & writer, CTX &ctx, UserCtx * userCtx = nullptr) {
    // validators::validators_detail::validator_state<Opts, ObjT> validatorsState;
    // if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::bool_parsing_finished>(obj, ctx.validationCtx())) {
    //     return ctx.withSchemaError(writer);
    // }

    if(!writer.write_bool(obj)) {
        return ctx.withWriterError(writer);
    }
    return true;
}


template <class Opts, class ObjT, writer::WriterLike Writer, class CTX, class UserCtx = void>
    requires static_schema::NumberLike<ObjT>
constexpr bool SerializeNonNullValue(const ObjT& obj, Writer & writer, CTX &ctx, UserCtx * userCtx = nullptr) {

    // validators::validators_detail::validator_state<Opts, ObjT> validatorsState;
    // if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::number_parsing_finished>(obj, ctx.validationCtx())) {
    //     return ctx.withSchemaError(writer);
    // }

    if(!writer.write_number(obj)) {
        return ctx.withWriterError(writer);
    }
    return true;
}


template <class Opts, class ObjT, writer::WriterLike Writer, class CTX, class UserCtx = void>
    requires static_schema::SerializableStringLike<ObjT>
constexpr bool SerializeNonNullValue(const ObjT& obj, Writer & writer, CTX &ctx, UserCtx * userCtx = nullptr) {

    // Use string_read_cursor for unified string serialization
    using Cursor = static_schema::string_read_cursor<ObjT>;
    Cursor cursor{obj};
    cursor.reset();
    
    stream_read_result res = cursor.read_more();
    while (res == stream_read_result::value) {
        if (!writer.write_string(cursor.data(), cursor.size())) {
            return ctx.withWriterError(writer);
        }
        res = cursor.read_more();
    }
    
    if (res == stream_read_result::error) {
        return ctx.withError(SerializeError::INPUT_STREAM_ERROR, writer);
    }
    return true;
}

template <class Opts, class ObjT, writer::WriterLike Writer, class CTX, class UserCtx = void>
    requires static_schema::SerializableArrayLike<ObjT>
constexpr bool SerializeNonNullValue(const ObjT& obj, Writer & writer, CTX &ctx, UserCtx * userCtx = nullptr) {
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
    typename Writer::ArrayFrame fr;
    if(!writer.write_array_begin(cursor.size(), fr)) {
        return ctx.withWriterError(writer);
    }

    cursor.reset();
    stream_read_result res;
    res = cursor.read_more();

    while(res != stream_read_result::end) {

        if(res == stream_read_result::error) {
            return ctx.withError(SerializeError::INPUT_STREAM_ERROR, writer);
        }

        const auto &ch = cursor.get();

        using Meta = options::detail::annotation_meta_getter<typename FH::element_type>;
        if(!SerializeValue<typename Meta::options>(Meta::getRef(ch), writer, ctx, userCtx)) {
            return false;
        }
        res = cursor.read_more();
        if(res == stream_read_result::error) {
            return ctx.withError(SerializeError::INPUT_STREAM_ERROR, writer);
        }
        if(res != stream_read_result::end) {
            if(!writer.advance_after_value(fr)) {
                return ctx.withWriterError(writer);
            }
        }
    }
    if(!writer.write_array_end(fr)) {
        return ctx.withWriterError(writer);
    }

    return true;
}

template <class Opts, class ObjT, writer::WriterLike Writer, class CTX, class UserCtx = void>
    requires static_schema::SerializableMapLike<ObjT>
constexpr bool SerializeNonNullValue(const ObjT& obj, Writer & writer, CTX &ctx, UserCtx * userCtx = nullptr) {

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

    typename Writer::MapFrame fr;
    if(!writer.write_map_begin(cursor.size(), fr)) {
        return ctx.withWriterError(writer);
    }

    cursor.reset();
    stream_read_result res = cursor.read_more();
    while(res != stream_read_result::end) {

        if(res == stream_read_result::error) {
            return ctx.withError(SerializeError::INPUT_STREAM_ERROR, writer);
        }
        const auto& key = cursor.get_key();
        const auto& value = cursor.get_value();

        using Meta = options::detail::annotation_meta_getter<typename FH::mapped_type>;
        if constexpr(static_schema::NullableSerializableValue<typename FH::mapped_type> &&
                      Opts::template has_option<options::detail::skip_nulls_tag>) {
            if(static_schema::isNull(value)) {
                res = cursor.read_more();
                continue;
            }
        }

        if constexpr(std::integral<typename FH::key_type>) {
            if(!writer.write_key_as_index(key)) {
                return ctx.withWriterError(writer);
            }
        } else {
            // Use string_read_cursor for unified key serialization
            using KeyCursor = static_schema::string_read_cursor<typename FH::key_type>;
            KeyCursor keyCursor{key};
            keyCursor.reset();
            
            stream_read_result keyRes = keyCursor.read_more();
            while (keyRes == stream_read_result::value) {
                if (!writer.write_string(keyCursor.data(), keyCursor.size())) {
                    return ctx.withWriterError(writer);
                }
                keyRes = keyCursor.read_more();
            }
            if (keyRes == stream_read_result::error) {
                return ctx.withError(SerializeError::INPUT_STREAM_ERROR, writer);
            }
        }

        if(!writer.move_to_value(fr)) {
            return ctx.withWriterError(writer);
        }
        
        // Serialize value

        if(!SerializeValue<typename Meta::options>(Meta::getRef(value), writer, ctx, userCtx)) {
            return false;
        }
        res = cursor.read_more();
        if(res == stream_read_result::error) {
            return ctx.withError(SerializeError::INPUT_STREAM_ERROR, writer);
        }
        if(res != stream_read_result::end) {
            if(!writer.advance_after_value(fr)) {
                return ctx.withWriterError(writer);
            }
        }
    }

    if(!writer.write_map_end(fr)) {
        return ctx.withWriterError(writer);
    }


    return true;
}


template <bool AsArray, bool indexesAsKeys, bool SkipNulls, std::size_t StructIndex, class Frame, class ObjT, writer::WriterLike Writer, class CTX, class UserCtx>
constexpr bool SerializeOneStructField(std::size_t & count, std::size_t & jfIndex, Frame & fr, ObjT& structObj, Writer & writer, CTX &ctx, UserCtx * userCtx = nullptr) {
    using Field   = introspection::structureElementTypeByIndex<StructIndex, ObjT>;
    using Meta =  options::detail::annotation_meta_getter<Field>;
    // using FieldOpts    = typename Meta::options;
    using FieldOpts = options::detail::aggregate_field_opts_getter<ObjT, StructIndex>;
    if constexpr (FieldOpts::template has_option<options::detail::exclude_tag>) {
        return true;
    } else {
        if constexpr(static_schema::NullableSerializableValue<Field> && SkipNulls){
            if(static_schema::isNull(introspection::getStructElementByIndex<StructIndex>(structObj))) {
                jfIndex ++;
                return true;
            }
        }
        if(count > 0) {
            if(!writer.advance_after_value(fr)) {
                return ctx.withWriterError(writer);
            }
        }
        if constexpr(!AsArray) {
            if constexpr(indexesAsKeys) {
                std::size_t int_key = struct_fields_helper::FieldsHelper<ObjT>::fieldIndexes[jfIndex].first;

                if(!writer.write_key_as_index(int_key)) {
                    return ctx.withWriterError(writer);
                }


            } else {
                if constexpr (FieldOpts::template has_option<options::detail::key_tag>) {
                    using KeyOpt = typename FieldOpts::template get_option<options::detail::key_tag>;
                    const auto & f =  KeyOpt::desc.toStringView();
                    if(!writer.write_string(f.data(), f.size())) {
                        return ctx.withWriterError(writer);
                    }
                } else {
                    const auto & f =   introspection::structureElementNameByIndex<StructIndex, ObjT>;
                    if(!writer.write_string(f.data(), f.size())) {
                        return ctx.withWriterError(writer);
                    }
                }
            }

            if(!writer.move_to_value(fr)) {
                return ctx.withWriterError(writer);
            }
        }

        count ++;
        jfIndex ++;
        if(!SerializeValue<FieldOpts>(Meta::getRef(
                                             introspection::getStructElementByIndex<StructIndex>(structObj)
                                           ), writer, ctx, userCtx)) {
            return false;
        }


        return true;
    }
}
template <bool AsArray, bool indexesAsKeys, bool skipNulls, class Frame, class ObjT, writer::WriterLike Writer, class CTX, class UserCtx, std::size_t... StructIndex>
constexpr bool SerializeStructFields(Frame &fr, const ObjT& structObj, Writer & writer, CTX &ctx, std::index_sequence<StructIndex...>, UserCtx * userCtx = nullptr) {
    std::size_t count = 0;
    std::size_t jfIndex = 0;

    std::size_t actualCount = 0;
    auto checkNull = [&](auto ic){
        constexpr std::size_t J = ic.value;
        using Field   = introspection::structureElementTypeByIndex<J, ObjT>;
        using FieldOpts = options::detail::aggregate_field_opts_getter<ObjT, J>;
        if constexpr (!FieldOpts::template has_option<options::detail::exclude_tag>) {
            if constexpr (!skipNulls || AsArray) {
                actualCount ++;
            } else {
                if constexpr(static_schema::NullableSerializableValue<Field>){
                    if(!static_schema::isNull(introspection::getStructElementByIndex<J>(structObj))) {
                        actualCount ++;
                    }
                } else {
                    actualCount ++;
                }
            }
        }
    };
    (checkNull(std::integral_constant<std::size_t, StructIndex>{}),...);
    if constexpr(std::is_same_v<Frame, typename Writer::MapFrame>) {
        if(!writer.write_map_begin( actualCount, fr)) {
            return ctx.withWriterError(writer);
        }
    } else {
        if(!writer.write_array_begin( actualCount, fr)) {
            return ctx.withWriterError(writer);
        }
    }

    return (
        SerializeOneStructField<AsArray, indexesAsKeys, skipNulls, StructIndex>(count, jfIndex, fr, structObj, writer, ctx, userCtx)
        && ...
        );
}

template <class Opts, class ObjT, writer::WriterLike Writer, class CTX, class UserCtx = void>
    requires static_schema::ObjectLike<ObjT>
constexpr bool SerializeNonNullValue(const ObjT& obj, Writer & writer, CTX &ctx, UserCtx * userCtx = nullptr) {

    typename Writer::MapFrame fr;




    if(!SerializeStructFields<false,
                               Opts::template has_option<options::detail::indexes_as_keys_tag> ||  struct_fields_helper::FieldsHelper<ObjT>::hasIntegerKeys,
                               Opts::template  has_option<options::detail::skip_nulls_tag>>(fr, obj, writer, ctx, std::make_index_sequence<introspection::structureElementsCount<ObjT>>{}, userCtx))
        return false;

    if(!writer.write_map_end(fr)) {
        return ctx.withWriterError(writer);
    }
    return true;

}

template <class Opts, class ObjT, writer::WriterLike Writer, class CTX, class UserCtx = void>
    requires static_schema::ObjectLike<ObjT>
        && Opts::template has_option<options::detail::as_array_tag> // special case for arrays destructuring
constexpr bool SerializeNonNullValue(const ObjT& obj, Writer & writer, CTX &ctx, UserCtx * userCtx = nullptr) {
    typename Writer::ArrayFrame fr;

    if(!SerializeStructFields<true, false, false>(fr, obj, writer, ctx, std::make_index_sequence<introspection::structureElementsCount<ObjT>>{}, userCtx))
        return false;

    if(!writer.write_array_end(fr)) {
        return ctx.withWriterError(writer);
    }

    return true;
}

template <class FieldOptions, static_schema::SerializableValue Field, writer::WriterLike Writer, class CTX, class UserCtx = void>
    requires (!static_schema::SerializeTransformerLike<Field> && !WireSinkLike<Field>)
constexpr  bool SerializeValue(const Field & obj, Writer & writer, CTX &ctx, UserCtx * userCtx = nullptr) {

    if constexpr (FieldOptions::template has_option<options::detail::exclude_tag>) {
        return true;
    } else if constexpr(static_schema::NullableSerializableValue<Field>) {

        if(static_schema::isNull(obj)) {
            if(!writer.write_null()) {
                return ctx.withWriterError(writer);
            } else {
                return true;
            }
        }
    }
    return SerializeNonNullValue<FieldOptions>(static_schema::getRef(obj), writer, ctx, userCtx);
}

// WireSink: First-class wire format emission
template <class FieldOptions, static_schema::SerializableValue Field, writer::WriterLike Writer, class CTX, class UserCtx = void>
    requires WireSinkLike<Field>
constexpr  bool SerializeValue(const Field & obj, Writer & writer, CTX &ctx, UserCtx * userCtx = nullptr) {
    if (!writer.output_from_sink(obj)) {
        return ctx.withWriterError(writer);
    }
    return true;
}

template <class FieldOptions, static_schema::SerializableValue Field, writer::WriterLike Writer, class CTX, class UserCtx = void>
    requires static_schema::SerializeTransformerLike<Field>
constexpr  bool SerializeValue(const Field & obj, Writer & writer, CTX &ctx, UserCtx * userCtx = nullptr) {
    using ObT = static_schema::serialize_transform_traits<Field>::wire_type;
    ObT ob;
    using Meta = options::detail::annotation_meta_getter<ObT>;
    if constexpr(!WireSinkLike<ObT>) {
        if(!obj.transform_to(ob)) {
            return ctx.withError(SerializeError::TRANSFORMER_ERROR, writer);
        } else {
            return serializer_details::SerializeValue<typename Meta::options>(Meta::getRef(ob), writer, ctx, userCtx);
        }
    } else {
        auto serializeFn = [&]<class ObjT>(const ObjT& srcObj) constexpr {
            auto res = SerializeWithWriter(srcObj, Writer::from_sink(ob), userCtx);
            if(!!res) ob.set_size(res.bytesWritten());
            return res;
        };
        if(!obj.transform_to(serializeFn)) {
            return ctx.withError(SerializeError::TRANSFORMER_ERROR, writer);
        } else {
            return serializer_details::SerializeValue<typename Meta::options>(Meta::getRef(ob), writer, ctx, userCtx);
        }

    }
}

} // namespace serializer_details



template <static_schema::SerializableValue InputObjectT, class UserCtx = void, writer::WriterLike Writer>
constexpr auto SerializeWithWriter(const InputObjectT & obj, Writer & writer, UserCtx * userCtx = nullptr) {
    serializer_details::SerializationContext<typename Writer::iterator_type, typename Writer::error_type> ctx(writer.current());
    using Meta = options::detail::annotation_meta_getter<InputObjectT>;

    serializer_details::SerializeValue<typename Meta::options>(Meta::getRef(obj), writer, ctx, userCtx);
    if(ctx.currentError() == SerializeError::NO_ERROR) {
        std::size_t bytesWritten = writer.finish();
        if(bytesWritten == std::size_t(-1)) {
            ctx.withWriterError(writer);
        } else {
            ctx.setBytesWritten(bytesWritten);
        }
    }

    return ctx.result();
}

template <static_schema::SerializableValue InputObjectT, class UserCtx = void, writer::WriterLike Writer>
constexpr auto SerializeWithWriter(const InputObjectT & obj, Writer && writer, UserCtx * userCtx = nullptr) {
    return SerializeWithWriter(obj, writer, userCtx);
}

template <static_schema::SerializableValue InputObjectT, CharOutputIterator It, CharSentinelForOut<It> Sent, class UserCtx = void, class Writer = JsonIteratorWriter<It, Sent>>
constexpr SerializeResult<It, typename Writer::error_type> Serialize(const InputObjectT & obj, It begin, const Sent & end, UserCtx * userCtx = nullptr) {
    Writer writer(begin, end);
    return SerializeWithWriter(obj, writer, userCtx);
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

template<static_schema::SerializableValue InputObjectT, class UserCtx = void>
constexpr auto Serialize(const InputObjectT& obj, std::string& out, UserCtx * ctx = nullptr)
{
    using io_details::limitless_sentinel;

    out.clear();

    auto it  = std::back_inserter(out);
    limitless_sentinel end{};

    return Serialize(obj, it, end, ctx);  // calls the iterator-based core
}
#endif

template<static_schema::SerializableValue InputObjectT, WireSinkLike Sink, class UserCtx = void>
constexpr auto Serialize(const InputObjectT& obj, Sink& sink, UserCtx * userCtx = nullptr) {
    sink.clear();
    auto it = sink.data();
    auto end = sink.data() + sink.max_size();
    auto result = Serialize(obj, it, end, userCtx);
    if (result) {  // operator bool() checks for success
        std::size_t written = it - sink.data();
        sink.set_size(written);
    }
    return result;
}


template <class T>
requires (!static_schema::SerializableValue<T>)
constexpr auto Serialize(T obj, auto C) {
    static_assert(static_schema::detail::always_false<T>::value,
                  "[[[ JsonFusion ]]] T is not a supported JsonFusion serializable value model type.\n"
                  "see SerializableValue concept for full rules");
}


template <class T>
    requires (!static_schema::SerializableValue<T>)
constexpr auto Serialize(T obj, auto C, auto S) {
    static_assert(static_schema::detail::always_false<T>::value,
                  "[[[ JsonFusion ]]] T is not a supported JsonFusion serializable value model type.\n"
                  "see SerializableValue concept for full rules");
}





} // namespace JsonFusion
