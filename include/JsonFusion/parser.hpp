#pragma once

#include <array>
// #include <cstdint>
#include <string_view>
#include <utility>  // std::declval
// #include <type_traits>
#include <algorithm>
#include <limits>
#include <bitset>

#include "static_schema.hpp"
#include "options.hpp"
#include "validators.hpp"

#include "struct_introspection.hpp"
#include "json_path.hpp"
#include "string_search.hpp"
#include "tokenizer.hpp"
#include "parse_errors.hpp"
#include "parse_result.hpp"

namespace JsonFusion {

constexpr bool allowed_dynamic_error_stack() {
#ifdef JSONFUSION_ALLOW_DYNAMIC_ERROR_STACK
    return true;
#else
    return false;
#endif
}


namespace  parser_details {


template <class InpIter, std::size_t SchemaDepth, bool SchemaHasMaps>
class DeserializationContext {

    ParseError error = ParseError::NO_ERROR;
    InpIter m_pos{};
    validators::validators_detail::ValidationCtx _validationCtx;

    static_assert(SchemaDepth != schema_analyzis::SCHEMA_UNBOUNDED || allowed_dynamic_error_stack(),
                  "JsonFusion: schema appears recursive / unbounded depth, "
                  "but dynamic error stack is not allowed. "
                  "Either refactor the schema or enable dynamic error stack support via JSONFUSION_ALLOW_DYNAMIC_ERROR_STACK macro.");


    using PathT = json_path::JsonPath<SchemaDepth, SchemaHasMaps>;
    PathT currentPath;
    using PathElementT = PathT::PathElementT;


public:
    struct PathGuard {
        DeserializationContext & ctx;

        constexpr ~PathGuard() {
            if(ctx.error == ParseError::NO_ERROR)
                ctx.currentPath.pop();
        }
    };


    constexpr void setError(ParseError err, InpIter pos) {
        error = err;
        m_pos = pos;
    }
    constexpr ParseError currentError(){return error;}

    constexpr ParseResult<InpIter, SchemaDepth, SchemaHasMaps> result() const {
        return ParseResult<InpIter, SchemaDepth, SchemaHasMaps>(error, _validationCtx.result(), m_pos, currentPath);
    }
    constexpr validators::validators_detail::ValidationCtx & validationCtx() {return _validationCtx;}


    constexpr PathGuard getArrayItemGuard(std::size_t index) {
        currentPath.push_child({index});
        return PathGuard{*this};
    }
    constexpr PathGuard getMapItemGuard(std::string_view key, bool is_static = true) {
        currentPath.push_child({std::numeric_limits<std::size_t>::max(), key, is_static});
        return PathGuard{*this};
    }
};


template <class Opts, class ObjT, json_reader::ReaderLike Tokenizer, class CTX, class UserCtx = void>
    requires static_schema::JsonBool<ObjT>
constexpr bool ParseNonNullValue(ObjT & obj, Tokenizer & reader, CTX &ctx, UserCtx * userCtx = nullptr) {
    if (json_reader::TryParseStatus st = reader.read_bool(obj); st == json_reader::TryParseStatus::error) {
        ctx.setError(reader.getError(), reader.current());
        return false;
    } else if (st == json_reader::TryParseStatus::no_match) {
        ctx.setError(ParseError::NON_BOOL_JSON_IN_BOOL_VALUE, reader.current());
        return false;
    }

    validators::validators_detail::validator_state<Opts, ObjT> validatorsState;
    if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::bool_parsing_finished>(obj, ctx.validationCtx())) {
        ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, reader.current());
        return false;
    } else {
        return true;
    }
}


// Strategy: custom integer parsing (no deps), delegated float parsing (configurable)

template <class Opts, class ObjT, json_reader::ReaderLike Tokenizer, class CTX, class UserCtx = void>
    requires static_schema::JsonNumber<ObjT>
constexpr bool ParseNonNullValue(ObjT& obj, Tokenizer & reader, CTX &ctx, UserCtx * userCtx = nullptr) {
    if (json_reader::TryParseStatus st = reader.template read_number<ObjT>(obj);
                st == json_reader::TryParseStatus::error) {
        ctx.setError(reader.getError(), reader.current());
        return false;
    }else if (st == json_reader::TryParseStatus::no_match) {
        ctx.setError(ParseError::FLOAT_VALUE_IN_INTEGER_STORAGE, reader.current());
        return false;
    }

    validators::validators_detail::validator_state<Opts, ObjT> validatorsState;
    if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::number_parsing_finished>(obj, ctx.validationCtx())) {
        ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, reader.current());
        return false;
    }

    return true;
}
constexpr std::size_t STRING_CHUNK_SIZE = 64;

template<class Reader, class Cont>
constexpr bool read_json_string_into(
    Reader&      reader,
    Cont&        out,
    std::size_t  max_len,   // validator limit or SIZE_MAX
    ParseError&         err,
    std::size_t& outSize,
    bool         read_rest_on_overflow = false
    )
{
    std::size_t total = 0;

    // Pre-reserve for dynamic containers to minimize reallocations.
    if constexpr (false && JsonFusion::static_schema::DynamicContainerTypeConcept<Cont>) {
        const std::size_t reserve_size =
            (max_len == std::numeric_limits<std::size_t>::max())
                ? STRING_CHUNK_SIZE
                : max_len;
        out.reserve(reserve_size);
    }

    for (;;) {
        // Remaining bytes we are *allowed* to store into the container.
        const std::size_t remaining = max_len - total;
        if (remaining == 0) {
            if (read_rest_on_overflow) {
                // Consume and discard the rest of the string so the reader ends
                // in a sane state (past the closing quote).
                char scratch[STRING_CHUNK_SIZE];

                for (;;) {
                    JsonFusion::json_reader::StringChunkResult rest =
                        reader.read_string_chunk(scratch, STRING_CHUNK_SIZE);

                    switch (rest.status) {
                    case JsonFusion::json_reader::StringChunkStatus::no_match:
                        // Shouldn't happen mid-string, but treat as ill-formed.
                        err = JsonFusion::ParseError::ILLFORMED_STRING;
                        return false;

                    case JsonFusion::json_reader::StringChunkStatus::error:
                        err = reader.getError();
                        return false;

                    case JsonFusion::json_reader::StringChunkStatus::ok:
                        break;
                    }

                    if (rest.bytes_written > STRING_CHUNK_SIZE) {
                        err = JsonFusion::ParseError::ILLFORMED_STRING;
                        return false;
                    }

                    if (rest.done) {
                        // We’ve reached the end of the JSON string.
                        err = JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW;
                        // outSize stays at the number of bytes we actually stored.
                        outSize = total;
                        return false;
                    }

                    if (rest.bytes_written == 0) {
                        // Reader made no progress but not done → malformed.
                        err = JsonFusion::ParseError::ILLFORMED_STRING;

                        return false;
                    }
                }
            } else {
                // Old behavior: fail immediately at overflow, leaving reader
                // mid-string.
                err = JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW;
                return false;
            }
        }

        const std::size_t ask =
            remaining < STRING_CHUNK_SIZE ? remaining : STRING_CHUNK_SIZE;

        JsonFusion::json_reader::StringChunkResult res;

        if constexpr (!JsonFusion::static_schema::DynamicContainerTypeConcept<Cont>) {
            res = reader.read_string_chunk(out + total, ask);

        } else if constexpr (std::is_same_v<typename Cont::value_type, char>) {
            // Dynamic string-like container: use a stack buffer + append
            char buf[STRING_CHUNK_SIZE];
            res = reader.read_string_chunk(buf, ask);

            switch (res.status) {
            case JsonFusion::json_reader::StringChunkStatus::ok:
                break;
            case JsonFusion::json_reader::StringChunkStatus::no_match:
                err = JsonFusion::ParseError::NON_STRING_IN_STRING_STORAGE;
                return false;
            case JsonFusion::json_reader::StringChunkStatus::error:
                err = reader.getError();
                return false;
            }

            if (res.bytes_written > ask) {
                err = JsonFusion::ParseError::ILLFORMED_STRING;
                return false;
            }

            if (res.bytes_written > 0) {
                out.append(buf, res.bytes_written);
                total += res.bytes_written;
            }

            if (res.done) {
                outSize = total;
                return true;
            }

            if (res.bytes_written == 0) {
                err = JsonFusion::ParseError::ILLFORMED_STRING;
                return false;
            }

            continue; // go to next chunk
        }else {
            // Dynamic non-char container (unlikely here, but keep it generic)
            out.resize(total + ask);
            res = reader.read_string_chunk(out.data() + total, ask);
        }

        switch (res.status) {
        case JsonFusion::json_reader::StringChunkStatus::no_match:
            // We expected a string here and didn't get one.
            err = JsonFusion::ParseError::NON_STRING_IN_STRING_STORAGE;
            return false;

        case JsonFusion::json_reader::StringChunkStatus::error:
            // Reader already set a specific error.
            err = reader.getError();
            return false;

        case JsonFusion::json_reader::StringChunkStatus::ok:
            break;
        }

        // Defensive check: reader must not overrun the supplied chunk.
        if (res.bytes_written > ask) {
            err = JsonFusion::ParseError::ILLFORMED_STRING;

            return false;
        }

        total += res.bytes_written;

        if (res.done) {
            // String fully consumed.
            outSize = total;
            if constexpr (JsonFusion::static_schema::DynamicContainerTypeConcept<Cont>) {
                out.resize(total);
            }
            return true;
        }

        // Safety net: if reader made no progress and says not done, bail
        // to avoid accidental infinite loops if the implementation ever regresses.
        if (res.bytes_written == 0) {
            err = JsonFusion::ParseError::ILLFORMED_STRING;
            return false;
        }

        // Otherwise: not done, made progress → loop again.
    }

    // Unreachable, but keeps compilers happy.
    // If we ever fall through, treat it as a malformed string.
    err = JsonFusion::ParseError::ILLFORMED_STRING;
    return false;
}

template <class Opts, class ObjT, json_reader::ReaderLike Tokenizer, class CTX, class UserCtx = void>
    requires static_schema::JsonString<ObjT>
constexpr bool ParseNonNullValue(ObjT& obj, Tokenizer & reader, CTX &ctx, UserCtx * userCtx = nullptr) {

    validators::validators_detail::validator_state<Opts, ObjT> validatorsState;

    //TODO extract MAX_SIZE from validators here
    constexpr std::size_t MAX_SIZE = std::numeric_limits<std::size_t>::max();
    std::size_t parsedSize = 0;
    ParseError err{ParseError::NO_ERROR};
    if constexpr (!static_schema::DynamicContainerTypeConcept<ObjT>) {
        char * b = static_schema::static_string_traits<ObjT>::data(obj);
        if(!read_json_string_into(reader, b, std::min(
                                                  static_schema::static_string_traits<ObjT>:: max_size(obj),
                                MAX_SIZE), err, parsedSize)) {
            ctx.setError(err, reader.current());
            return false;
        }
        b[parsedSize] = 0;
    } else {
        obj.clear();
        if(!read_json_string_into(reader, obj, MAX_SIZE, err, parsedSize)) {
            ctx.setError(err, reader.current());
            return false;
        }
    }

    if constexpr (static_schema::DynamicContainerTypeConcept<ObjT>) {
        if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::string_parsing_finished>(obj, ctx.validationCtx(),
                    std::string_view(obj.data(), obj.data() + parsedSize))) {
            ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, reader.current());
            return false;
        }
    } else {
        char * b = static_schema::static_string_traits<ObjT>::data(obj);
        if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::string_parsing_finished>(obj, ctx.validationCtx(),
                    std::string_view(b, b + parsedSize))) {
            ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, reader.current());
            return false;
        }
    }

    return true;
}

template <class Opts, class ObjT, json_reader::ReaderLike Tokenizer, class CTX, class UserCtx = void>
    requires static_schema::JsonParsableArray<ObjT>
constexpr bool ParseNonNullValue(ObjT& obj, Tokenizer & reader, CTX &ctx, UserCtx * userCtx = nullptr) {

    typename Tokenizer::ArrayFrame fr;
    if(!reader.read_array_begin(fr)) {
        ctx.setError(ParseError::NON_ARRAY_IN_ARRAY_LIKE_VALUE, reader.current());
        return false;
    }

    std::size_t parsed_items_count = 0;
    bool has_trailing_comma = false;

    using FH   = static_schema::array_write_cursor<ObjT>;

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
    validators::validators_detail::validator_state<Opts, ObjT> validatorsState;

    while(true) {
        if(json_reader::TryParseStatus st = reader.read_array_end(fr); st == json_reader::TryParseStatus::error) {
            cursor.finalize(false);
            ctx.setError(reader.getError(), reader.current());
            return false;
        } else if(st == json_reader::TryParseStatus::no_match) {
            if(parsed_items_count > 0 && !has_trailing_comma) {
                ctx.setError(ParseError::ILLFORMED_ARRAY, reader.current());
                cursor.finalize(false);
                return false;
            }
        } else {
            if(has_trailing_comma) {
                ctx.setError(ParseError::ILLFORMED_ARRAY, reader.current());
                cursor.finalize(false);
                return false;
            }
            if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::array_parsing_finished>(obj, ctx.validationCtx(), parsed_items_count)) {
                ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, reader.current());
                cursor.finalize(false);
                return false;
            }
            cursor.finalize(true);
            return true;
        }

        stream_write_result alloc_r = cursor.allocate_slot();
        if(alloc_r != stream_write_result::slot_allocated) {
            if(alloc_r == stream_write_result::overflow ) {
                ctx.setError(ParseError::FIXED_SIZE_CONTAINER_OVERFLOW, reader.current());
                return false;
            } else {
                ctx.setError(ParseError::DATA_CONSUMER_ERROR, reader.current());
                return false;
            }
        }

        typename FH::element_type & newItem = cursor.get_slot();
        typename CTX::PathGuard guard = ctx.getArrayItemGuard(parsed_items_count);

        using Meta = options::detail::annotation_meta_getter<typename FH::element_type>;
        if(!ParseValue<typename Meta::options>(Meta::getRef(newItem), reader, ctx, userCtx)) {
            cursor.finalize_item(false);
            cursor.finalize(false);
            return false;
        }

        stream_write_result finalize_r = cursor.finalize_item(true);
        if(finalize_r != stream_write_result::value_processed) {
            if(finalize_r == stream_write_result::overflow) {
                ctx.setError(ParseError::DUPLICATE_KEY_IN_MAP, reader.current());
                return false;
            } else {
                ctx.setError(ParseError::DATA_CONSUMER_ERROR, reader.current());
                return false;
            }
        }


        parsed_items_count ++;
        if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::array_item_parsed>(obj, ctx.validationCtx(), parsed_items_count)) {
            ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, reader.current());
            cursor.finalize(false);
            return false;
        }

        if (!reader.consume_value_separator(fr, has_trailing_comma)) {
            ctx.setError(reader.getError(), reader.current());
            cursor.finalize(false);
            return false;
        }
    }
    cursor.finalize(false);
    return false;
}

template <class Opts, class ObjT, json_reader::ReaderLike Tokenizer, class CTX, class UserCtx = void>
    requires static_schema::JsonParsableMap<ObjT>
constexpr bool ParseNonNullValue(ObjT& obj, Tokenizer & reader, CTX &ctx, UserCtx * userCtx = nullptr) {

    typename Tokenizer::ObjectFrame obFrame;

    if(!reader.read_object_begin(obFrame)) {
        ctx.setError(ParseError::NON_OBJECT_IN_MAP_LIKE_VALUE, reader.current());
        return false;
    }

    std::size_t parsed_entries_count = 0;
    bool has_trailing_comma = false;

    using FH = static_schema::map_write_cursor<ObjT>;

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

    validators::validators_detail::validator_state<Opts, ObjT> validatorsState;
    while(true) {
        if(json_reader::TryParseStatus st = reader.read_object_end(obFrame); st == json_reader::TryParseStatus::error) {
            ctx.setError(reader.getError(), reader.current());
            cursor.finalize(false);
            return false;
        } else if(st == json_reader::TryParseStatus::no_match) {
            if(parsed_entries_count > 0 && !has_trailing_comma) {
                ctx.setError(ParseError::ILLFORMED_OBJECT, reader.current());
                cursor.finalize(false);
                return false;
            }
        } else {
            if(has_trailing_comma) {
                ctx.setError(ParseError::ILLFORMED_OBJECT, reader.current());
                cursor.finalize(false);
                return false;
            }
            if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::map_parsing_finished>(obj, ctx.validationCtx(), parsed_entries_count)) {
                ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, reader.current());
                cursor.finalize(false);
                return false;
            }
            cursor.finalize(true);
            return true;
        }
        
        // Allocate key slot
        stream_write_result alloc_r = cursor.allocate_key();
        if(alloc_r != stream_write_result::slot_allocated) {
            if(alloc_r == stream_write_result::overflow) {
                ctx.setError(ParseError::FIXED_SIZE_CONTAINER_OVERFLOW, reader.current());
                return false;
            } else {
                ctx.setError(ParseError::DATA_CONSUMER_ERROR, reader.current());
                return false;
            }
        }
        
        // Parse key as string with incremental validation
        typename FH::key_type& key = cursor.key_ref();
        std::string_view key_sv;
        if constexpr(!std::integral<typename FH::key_type>) {
            // Custom string parsing with incremental key validation
            std::size_t parsedSize = 0;

            //TODO extract MAX_SIZE from validators here
            constexpr std::size_t MAX_SIZE = std::numeric_limits<std::size_t>::max();

            ParseError err{ParseError::NO_ERROR};
            if constexpr (!static_schema::DynamicContainerTypeConcept<typename FH::key_type>) {
                char * b = static_schema::static_string_traits<typename FH::key_type>::data(key);
                if(!read_json_string_into(reader, b, std::min(
                                                          static_schema::static_string_traits<typename FH::key_type>::max_size(key),

                    MAX_SIZE), err, parsedSize)) {
                    ctx.setError(err, reader.current());
                    return false;
                }
                b[parsedSize] = 0;
            } else {
                if(!read_json_string_into(reader, key, MAX_SIZE, err, parsedSize)) {
                    ctx.setError(err, reader.current());
                    return false;
                }
            }


            // Emit key finished event
            key_sv = std::string_view(key.data(), key.data() + parsedSize);
            if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::map_key_finished>(obj, ctx.validationCtx(), key_sv)) {
                ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, reader.current());
                return false;
            }

        } else {
            if(!reader.read_key_as_index(key)) {
                ctx.setError(reader.getError(), reader.current());
                cursor.finalize(false);
                return false;
            }

            if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::map_key_finished>(obj, ctx.validationCtx(), key)) {
                ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, reader.current());
                cursor.finalize(false);
                return false;
            }
            key_sv = std::string_view(reinterpret_cast<char*>(&key), sizeof(key)); // TODO
        }


        if (!reader.consume_kv_separator(obFrame)) {
            ctx.setError(reader.getError(), reader.current());
            cursor.finalize(false);
            return false;
        }
        
        // Allocate value slot
        alloc_r = cursor.allocate_value_for_parsed_key();
        if(alloc_r != stream_write_result::slot_allocated) {
            if(alloc_r == stream_write_result::overflow) {
                ctx.setError(ParseError::FIXED_SIZE_CONTAINER_OVERFLOW, reader.current());
                return false;
            } else {
                ctx.setError(ParseError::DATA_CONSUMER_ERROR, reader.current());
                return false;
            }
        }
        
        // Parse value
        typename FH::mapped_type& value = cursor.value_ref();

        typename CTX::PathGuard guard = ctx.getMapItemGuard(key_sv, false);

        using Meta = options::detail::annotation_meta_getter<typename FH::mapped_type>;
        if(!ParseValue<typename Meta::options>(Meta::getRef(value), reader, ctx, userCtx)) {
            return false;
        }

        if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::map_value_parsed>(obj, ctx.validationCtx(), parsed_entries_count)) {
            ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, reader.current());
            return false;
        }

        // Finalize the key-value pair
        stream_write_result finalize_r = cursor.finalize_pair(true);
        if(finalize_r != stream_write_result::value_processed) {
            if(finalize_r == stream_write_result::error) {
                ctx.setError(ParseError::DUPLICATE_KEY_IN_MAP, reader.current());
                return false;
            } else {
                ctx.setError(ParseError::DATA_CONSUMER_ERROR, reader.current());
                return false;
            }
        }
        
        parsed_entries_count++;

        if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::map_entry_parsed>(obj, ctx.validationCtx(), parsed_entries_count)) {
            ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, reader.current());
            cursor.finalize(false);
            return false;
        }
        if (!reader.consume_value_separator(obFrame, has_trailing_comma)) {
            ctx.setError(reader.getError(), reader.current());
            cursor.finalize(false);
            return false;
        }
    }
    
    return false;
}



template<class T, std::size_t I>
static consteval bool fieldIsNotJSON() {
    using Opts    = options::detail::aggregate_field_opts_getter<T, I>;
    if constexpr (Opts::template has_option<options::detail::not_json_tag>) {
        return true;
    } else {
        return false;
    }
}

template<class T>
struct FieldsHelper {
    using FieldDescr = string_search::StringDescr;


    static constexpr std::size_t rawFieldsCount = introspection::structureElementsCount<T>;

    static constexpr std::size_t fieldsCount = []<std::size_t... I>(std::index_sequence<I...>) consteval{
        return (std::size_t{0} + ... + (!fieldIsNotJSON<T, I>() ? 1: 0));
    }(std::make_index_sequence<rawFieldsCount>{});



    template<std::size_t I>
    static consteval std::string_view fieldName() {
        using Opts    = options::detail::aggregate_field_opts_getter<T, I>;
        if constexpr (Opts::template has_option<options::detail::key_tag>) {
            using KeyOpt = typename Opts::template get_option<options::detail::key_tag>;
            return KeyOpt::desc.toStringView();
        } else {
            return introspection::structureElementNameByIndex<I, T>;
        }
    }

    static constexpr std::array<FieldDescr, fieldsCount> fieldIndexesToFieldNames =
        []<std::size_t... I>(std::index_sequence<I...>) consteval {
            std::array<FieldDescr, fieldsCount> arr;
            std::size_t index = 0;
            auto add_one = [&](auto ic) consteval {
                constexpr std::size_t J = decltype(ic)::value;
                if constexpr (!fieldIsNotJSON<T, J>()) {
                    arr[index++] = FieldDescr{ fieldName<J>(), J };
                }
            };
            (add_one(std::integral_constant<std::size_t, I>{}), ...);
            // std::ranges::sort(arr, {}, &FieldDescr::name);

            return arr;
        }(std::make_index_sequence<rawFieldsCount>{});

    static constexpr bool fieldsAreUnique = [](std::array<FieldDescr, fieldsCount> sortedArr) consteval{
        return std::ranges::adjacent_find(sortedArr, {}, &FieldDescr::name) == sortedArr.end();
    }(fieldIndexesToFieldNames);

    static constexpr std::size_t maxFieldNameLength = []() consteval {
        std::size_t maxLen = 0;
        for (const auto& field : fieldIndexesToFieldNames) {
            if (field.name.size() > maxLen) {
                maxLen = field.name.size();
            }
        }
        return maxLen;
    }();

    static consteval std::size_t indexInSortedByName(std::string_view name) {
        for(std::size_t i = 0; i < fieldsCount; i++) {
            if(fieldIndexesToFieldNames[i].name == name) {
                return i;
            }
        }
        return -1;
    }


    // static constexpr string_search::PerfectHashDFA<fieldsCount, maxFieldNameLength> dfa = []() consteval {
    //     return string_search::PerfectHashDFA<fieldsCount, maxFieldNameLength>(fieldIndexesToFieldNames);
    // }();
};


template<class StructT, std::size_t StructIndex>
using StructFieldMeta = options::detail::annotation_meta_getter<
    introspection::structureElementTypeByIndex<StructIndex, StructT>
>;
template <class ObjT, json_reader::ReaderLike Tokenizer, class CTX, class UserCtx, std::size_t... StructIndex>
    requires static_schema::JsonObject<ObjT>
constexpr bool ParseStructField(ObjT& structObj, Tokenizer & reader, CTX &ctx, std::index_sequence<StructIndex...>, std::size_t requiredIndex, UserCtx * userCtx = nullptr) {
    bool ok = false;
    (
        (requiredIndex == StructIndex
             ? (
                ok = ParseValue< options::detail::aggregate_field_opts_getter<ObjT, StructIndex>>(
                       StructFieldMeta<ObjT, StructIndex>::getRef(
                           introspection::getStructElementByIndex<StructIndex>(structObj)
                           ),
                       reader, ctx, userCtx
                       )



                , 0)
             : 0),
        ...
        );
    return ok;
}

template<std::size_t I, class ObjT, class Tokenizer, class CTX, class UserCtx>
constexpr bool parse_struct_field_one(
    ObjT& structObj,
    Tokenizer& reader,
    CTX& ctx,
    UserCtx* userCtx)
{
    using Opts = options::detail::aggregate_field_opts_getter<ObjT, I>;
    auto& field = introspection::getStructElementByIndex<I>(structObj);
    return ParseValue<Opts>(
        StructFieldMeta<ObjT, I>::getRef(field),
        reader, ctx, userCtx
    );
}

template <class Opts, class ObjT, json_reader::ReaderLike Tokenizer, class CTX, class UserCtx = void>
    requires static_schema::JsonObject<ObjT>
constexpr bool ParseNonNullValue(ObjT& obj, Tokenizer & reader, CTX &ctx, UserCtx * userCtx = nullptr) {
    using FH = FieldsHelper<ObjT>;
    static_assert(FH::fieldsAreUnique, "[[[ JsonFusion ]]] Fields are not unique");

    typename Tokenizer::ObjectFrame obFrame;
    if(!reader.read_object_begin(obFrame)) {
        ctx.setError(ParseError::NON_OBJECT_IN_MAP_LIKE_VALUE, reader.current());
        return false;
    }

    bool has_trailing_comma = false;
    bool isFirst = true;

    std::bitset<FH::fieldsCount> parsedFieldsByIndex{};
    validators::validators_detail::validator_state<Opts, ObjT> validatorsState;


    while(true) {
        if(json_reader::TryParseStatus st = reader.read_object_end(obFrame); st == json_reader::TryParseStatus::error) {
            ctx.setError(reader.getError(), reader.current());
            return false;
        } else if(st == json_reader::TryParseStatus::no_match) {
            if(!isFirst && !has_trailing_comma) {
                ctx.setError(ParseError::ILLFORMED_OBJECT, reader.current());
                return false;
            }
        } else {
            if(has_trailing_comma) {
                ctx.setError(ParseError::ILLFORMED_OBJECT, reader.current());
                return false;
            }
         
            if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::object_parsing_finished>(obj, ctx.validationCtx(), parsedFieldsByIndex, FH{})) {
                ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, reader.current());
                return false;
            }

            return true;
        }

        std::size_t structIndex = -1;
        std::size_t arrayIndex = -1;
        std::string_view key_sv;

        if constexpr(Opts::template has_option<options::detail::indexes_as_keys_tag>) {
            if(!reader.read_key_as_index(arrayIndex)) {
                ctx.setError(reader.getError(), reader.current());
                return false;
            }

            key_sv = std::string_view(reinterpret_cast<char*>(&arrayIndex), sizeof(arrayIndex)); // TODO
            if(arrayIndex >= FH::fieldIndexesToFieldNames.size()) {
                arrayIndex = -1;
            } else {
                structIndex = FH::fieldIndexesToFieldNames[arrayIndex].originalIndex;
            }
        } else {
            string_search::AdaptiveStringSearch<Opts::template has_option<options::detail::binary_fields_search_tag>, FH::maxFieldNameLength> searcher{
                FH::fieldIndexesToFieldNames.data(), FH::fieldIndexesToFieldNames.data() + FH::fieldIndexesToFieldNames.size()
            };
            char * b = searcher.buffer();

            ParseError err{ParseError::NO_ERROR};
            if(!read_json_string_into(reader, b ,FH::maxFieldNameLength, err, searcher.current_length(), true)) {
                if(err == ParseError::NON_STRING_IN_STRING_STORAGE) {
                    ctx.setError(ParseError::ILLFORMED_OBJECT, reader.current());
                } else if (err == ParseError::FIXED_SIZE_CONTAINER_OVERFLOW) {
                    ctx.setError(ParseError::NO_ERROR, reader.current());
                    searcher.set_overflow();
                } else {
                    ctx.setError(err, reader.current());
                    return false;
                }
            }
            auto res = searcher.result();
            if( searcher.result() != FH::fieldIndexesToFieldNames.end()) {
                arrayIndex = res - FH::fieldIndexesToFieldNames.begin();
                structIndex = res->originalIndex;
                key_sv = res->name;
            }

        }
        if (!reader.consume_kv_separator(obFrame)) {
            ctx.setError(reader.getError(), reader.current());
            return false;
        }

        if(arrayIndex == std::size_t(-1)) {
            if constexpr (Opts::template has_option<options::detail::allow_excess_fields_tag>) {
                using Opt = typename Opts::template get_option<options::detail::allow_excess_fields_tag>;

                if(!reader.template skip_json_value<Opt::SkipDepthLimit>()) {
                    ctx.setError(reader.getError(), reader.current());
                    return false;
                }
            } else {
                ctx.setError(ParseError::EXCESS_FIELD, reader.current());
                return false;
            }
        } else {
            if(parsedFieldsByIndex[arrayIndex] == true) {
                ctx.setError(ParseError::ILLFORMED_OBJECT, reader.current());
                return false;
            }


            typename CTX::PathGuard guard = ctx.getMapItemGuard(key_sv);

            if(!ParseStructField(obj, reader, ctx, std::make_index_sequence<FH::rawFieldsCount>{}, structIndex, userCtx)) {
                return false;
            }else {
                parsedFieldsByIndex[arrayIndex] = true;
            }
        }




        isFirst = false;
        if (!reader.consume_value_separator(obFrame, has_trailing_comma)) {
            ctx.setError(reader.getError(), reader.current());
            return false;
        }
    }
    return false;
}


/* #### SPECIAL CASE FOR ARRAYS DESRTUCTURING #### */





template <class Opts, class ObjT, json_reader::ReaderLike Tokenizer, class CTX, class UserCtx = void>
    requires static_schema::JsonObject<ObjT>
             &&
             Opts::template has_option<options::detail::as_array_tag>
constexpr bool ParseNonNullValue(ObjT& obj, Tokenizer & reader, CTX &ctx, UserCtx * userCtx = nullptr) {
    typename Tokenizer::ArrayFrame fr;
    if(!reader.read_array_begin(fr)) {
        ctx.setError(ParseError::NON_ARRAY_IN_DESTRUCTURED_STRUCT, reader.current());
        return false;
    }

    std::size_t requiredIndex = 0;
    std::size_t parsed_items_count = 0;
    bool has_trailing_comma = false;

    constexpr std::size_t totalFieldsCount = introspection::structureElementsCount<ObjT>;
    validators::validators_detail::validator_state<Opts, ObjT> validatorsState;


    static constexpr std::array<bool, totalFieldsCount> isFieldSkipped = []<std::size_t... I>(std::index_sequence<I...>) {
        return std::array<bool, sizeof...(I)>{
            fieldIsNotJSON<ObjT, I>()...
        };
    }(std::make_index_sequence<totalFieldsCount>{});
    while(true) {
        if(json_reader::TryParseStatus st = reader.read_array_end(fr); st == json_reader::TryParseStatus::error) {
            ctx.setError(reader.getError(), reader.current());
            return false;
        } else if(st == json_reader::TryParseStatus::no_match) {
            if(parsed_items_count > 0 && !has_trailing_comma) {
                ctx.setError(ParseError::ILLFORMED_ARRAY, reader.current());
                return false;
            }
        } else {
            if(has_trailing_comma) {
                ctx.setError(ParseError::ILLFORMED_ARRAY, reader.current());
                return false;
            }

            while(requiredIndex < totalFieldsCount && isFieldSkipped[requiredIndex]) {
                requiredIndex ++;
            }

            if(requiredIndex!= totalFieldsCount) {
                ctx.setError(ParseError::ARRAY_DESTRUCTURING_SCHEMA_ERROR, reader.current());
                return false;
            }

            if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::descrtuctured_object_parsing_finished>(obj, ctx.validationCtx())) {
                ctx.setError(ParseError::SCHEMA_VALIDATION_ERROR, reader.current());
                return false;
            }

            return true;
        }

        while(requiredIndex < totalFieldsCount && isFieldSkipped[requiredIndex]) {
            requiredIndex ++;
        }

        if(requiredIndex >= totalFieldsCount) {
            ctx.setError(ParseError::ARRAY_DESTRUCTURING_SCHEMA_ERROR, reader.current());
            return false;
        }

        if(!ParseStructField(obj, reader, ctx, std::make_index_sequence<totalFieldsCount>{}, requiredIndex, userCtx)) {
            return false;
        }
        // if(!table[requiredIndex](obj, reader, ctx, userCtx)) {
        //     return false;
        // }
        parsed_items_count ++;
        requiredIndex ++;


        if (!reader.consume_value_separator(fr, has_trailing_comma)) {
            ctx.setError(reader.getError(), reader.current());
            return false;
        }
    }
    return false;
}

template <class FieldOptions, static_schema::JsonParsableValue Field, json_reader::ReaderLike Tokenizer, class CTX, class UserCtx = void>
    requires (!static_schema::ParseTransformer<Field>)
constexpr bool ParseValue(Field & field, Tokenizer & reader, CTX &ctx, UserCtx * userCtx = nullptr) {

    if constexpr (FieldOptions::template has_option<options::detail::not_json_tag>) {
        return false; // cannot parse non-json
    }else if constexpr (FieldOptions::template has_option<options::detail::skip_json_tag>) {
        using Opt = FieldOptions::template get_option<options::detail::skip_json_tag>;
        if(!reader.template skip_json_value<Opt::SkipDepthLimit>()) {
            ctx.setError(reader.getError(), reader.current());
            return false;
        } else {
            return true;
        }
    } else if constexpr(FieldOptions::template has_option<options::detail::json_sink_tag>) {
        using Opt = FieldOptions::template get_option<options::detail::json_sink_tag>;
        static_assert(static_schema::JsonString<Field>, "json_sink should be used with string-like types");
        if(!reader.template skip_json_value<Opt::SkipDepthLimit>(std::addressof(field), Opt::MaxStringLength)) {
            ctx.setError(reader.getError(), reader.current());
            return false;
        } else {
            return true;
        }
    } else {
        if(json_reader::TryParseStatus r = reader.skip_ws_and_read_null(); r == json_reader::TryParseStatus::ok) {
            if constexpr(static_schema::JsonNullableParsableValue<Field>) {
                static_schema::setNull(field);
                return true;
            } else {
                ctx.setError(ParseError::NULL_IN_NON_OPTIONAL, reader.current());
                return false;
            }
        } else if(r == json_reader::TryParseStatus::error) {
            ctx.setError(reader.getError(), reader.current());
            return false;
        } else {
            return ParseNonNullValue<FieldOptions>(static_schema::getRef(field), reader, ctx, userCtx);
        }
    }
}

template <class FieldOptions, static_schema::JsonParsableValue Field, json_reader::ReaderLike Tokenizer, class CTX, class UserCtx = void>
    requires static_schema::ParseTransformer<Field>
constexpr bool ParseValue(Field & field, Tokenizer & reader, CTX &ctx, UserCtx * userCtx = nullptr) {
    using StT = static_schema::parse_transform_traits<Field>::wire_type;
    StT ob;
    using Meta = options::detail::annotation_meta_getter<StT>;
    bool r = parser_details::ParseValue<typename Meta::options>(Meta::getRef(ob), reader, ctx, userCtx);
    if(r) {
        if(!field.transform_from(ob)) {
            ctx.setError(ParseError::TRANSFORMER_ERROR, reader.current());
            return false;
        }
    }
    return r;
}


} // namespace parser_details


template <static_schema::JsonParsableValue InputObjectT, class UserCtx = void, json_reader::ReaderLike Reader>
constexpr auto ParseWithReader(InputObjectT & obj, Reader & reader, UserCtx * userCtx = nullptr) {
    using Tr = ModelParsingTraits<InputObjectT>;
    using CtxT = parser_details::DeserializationContext<typename Reader::iterator_type, Tr::SchemaDepth, Tr::SchemaHasMaps>;

    CtxT ctx;

    using Meta = options::detail::annotation_meta_getter<InputObjectT>;

    parser_details::ParseValue<typename Meta::options>(Meta::getRef(obj), reader, ctx, userCtx);

    if(ctx.currentError() == ParseError::NO_ERROR) {
        reader.skip_whitespaces_till_the_end();
    }
    return ctx.result();
}

template <static_schema::JsonParsableValue InputObjectT, CharInputIterator It, CharSentinelFor<It> Sent, class UserCtx = void, class Reader = tokenizer::JsonIteratorReader<It, Sent>>
constexpr auto Parse(InputObjectT & obj, It begin, const Sent & end, UserCtx * userCtx = nullptr) {
    Reader reader(begin, end);
    return ParseWithReader(obj, reader, userCtx);
}


template<class InputObjectT, class ContainterT, class UserCtx = void>
    requires (!std::is_pointer_v<ContainterT>) && requires(const ContainterT& c) { c.begin(); c.end(); }
constexpr auto Parse(InputObjectT & obj, const ContainterT & c, UserCtx * userCtx = nullptr) {
    return Parse(obj, c.begin(), c.end(), userCtx);
}

template<static_schema::JsonParsableValue InputObjectT, class UserCtx = void>
constexpr auto Parse(InputObjectT& obj, std::string_view sv, UserCtx * userCtx = nullptr) {
    return Parse(obj, sv.data(), sv.data() + sv.size(), userCtx);
}


// string front-end TODO
// template<static_schema::JsonParsableValue InputObjectT, class UserCtx = void>
// constexpr auto Parse(InputObjectT& obj, const std::string & sv, UserCtx * userCtx = nullptr) {
//     return Parse(obj, sv.data(), sv.data()+ sv.size(), userCtx);
// }


template <class T>
    requires (!static_schema::JsonParsableValue<T>)
constexpr auto  Parse(T obj, auto, auto) {
    static_assert(static_schema::detail::always_false<T>::value,
                  "[[[ JsonFusion ]]] T is not a supported JsonFusion parsable value model type.\n"
                  "see JsonParsableValue concept for full rules");
}

template <class T>
    requires (!static_schema::JsonParsableValue<T>)
constexpr auto Parse(T obj, auto) {
    static_assert(static_schema::detail::always_false<T>::value,
                  "[[[ JsonFusion ]]] T is not a supported JsonFusion parsable value model type.\n"
                  "see JsonParsableValue concept for full rules");
}


} // namespace JsonFusion
