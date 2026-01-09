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
#include "wire_sink.hpp"

#include "struct_introspection.hpp"
#include "path.hpp"
#include "string_search.hpp"
#include "json.hpp"
#include "errors.hpp"
#include "parse_result.hpp"
#include "struct_fields_helper.hpp"

namespace JsonFusion {

constexpr bool allowed_dynamic_error_stack() {
#ifdef JSONFUSION_ALLOW_DYNAMIC_ERROR_STACK
    return true;
#else
    return false;
#endif
}


namespace  parser_details {


template <class InpIter, std::size_t SchemaDepth, bool SchemaHasMaps, class ReaderError>
class DeserializationContext {
    ReaderError reader_error = {};
    ParseError error = ParseError::NO_ERROR;
    InpIter m_pos{};
    validators::validators_detail::ValidationCtx _validationCtx;

    static_assert(SchemaDepth != schema_analyzis::SCHEMA_UNBOUNDED || allowed_dynamic_error_stack(),
                  "JsonFusion: schema appears recursive / unbounded depth, "
                  "but dynamic error stack is not allowed. "
                  "Either refactor the schema or enable dynamic error stack support via JSONFUSION_ALLOW_DYNAMIC_ERROR_STACK macro.");


    using PathT = path::Path<SchemaDepth, SchemaHasMaps>;
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


    constexpr bool withParseError(ParseError err, const reader::ReaderLike auto & reader) {
        error = err;
        if(err == ParseError::NO_ERROR) {
            error = ParseError::READER_ERROR;
        }
        reader_error = reader.getError();
        m_pos = reader.current();
        return false;
    }

    constexpr bool withReaderError(const reader::ReaderLike auto & reader) {
        error = ParseError::READER_ERROR;
        reader_error = reader.getError();
        m_pos = reader.current();
        return false;
    }
    constexpr bool withSchemaError(const reader::ReaderLike auto & reader) {
        error = ParseError::SCHEMA_VALIDATION_ERROR;
        m_pos = reader.current();
        return false;
    }
    constexpr ParseError currentError(){return error;}

    constexpr ParseResult<InpIter, ReaderError, SchemaDepth, SchemaHasMaps> result() const {
        return ParseResult<InpIter, ReaderError, SchemaDepth, SchemaHasMaps>(error, reader_error, _validationCtx.result(), m_pos, currentPath);
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


template <class Opts, class ObjT, reader::ReaderLike Reader, class CTX, class UserCtx = void>
    requires static_schema::BoolLike<ObjT>
constexpr bool ParseNonNullValue(ObjT & obj, Reader & reader, CTX &ctx, UserCtx * userCtx = nullptr) {
    if (reader::TryParseStatus st = reader.read_bool(obj); st == reader::TryParseStatus::error) {
        return ctx.withReaderError(reader);
    } else if (st == reader::TryParseStatus::no_match) {
        return ctx.withParseError(ParseError::NON_BOOL_IN_BOOL_VALUE, reader);
    }

    validators::validators_detail::validator_state<Opts, ObjT> validatorsState;
    if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::bool_parsing_finished>(obj, ctx.validationCtx())) {
        return ctx.withSchemaError(reader);
    } else {
        return true;
    }
}


template <class Opts, class ObjT, reader::ReaderLike Reader, class CTX, class UserCtx = void>
    requires static_schema::NumberLike<ObjT>
constexpr bool ParseNonNullValue(ObjT& obj, Reader & reader, CTX &ctx, UserCtx * userCtx = nullptr) {
    if (reader::TryParseStatus st = reader.template read_number<ObjT>(obj);
                st == reader::TryParseStatus::error) {
        return ctx.withReaderError(reader);
    }else if (st == reader::TryParseStatus::no_match) {
        return ctx.withParseError(ParseError::NON_NUMERIC_IN_NUMERIC_STORAGE, reader);
    }

    validators::validators_detail::validator_state<Opts, ObjT> validatorsState;
    if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::number_parsing_finished>(obj, ctx.validationCtx())) {
        return ctx.withSchemaError(reader);
    }

    return true;
}


constexpr std::size_t STRING_CHUNK_SIZE = 64;

// Helper: read string using cursor abstraction
// Cursor provides: prepare_write(), write_ptr(), commit(), finalize(), max_capacity()
// This unified interface works for both static buffers (zero-copy) and dynamic strings (buffered append)
template<class Reader, class Cursor>
__attribute__((noinline)) constexpr bool read_string_with_cursor(
    Reader&      reader,
    Cursor&      cursor,
    std::size_t  max_len,   // validator limit or SIZE_MAX
    ParseError&  err,
    bool         read_rest_on_overflow = false
)
{
    for (;;) {
        // Check if we've hit the validator limit
        const std::size_t current_size = cursor.size();
        if (current_size >= max_len) {
            if (read_rest_on_overflow) {
                // Consume and discard the rest of the string so the reader ends
                // in a sane state (past the closing quote).
                char scratch[STRING_CHUNK_SIZE];

                for (;;) {
                    JsonFusion::reader::StringChunkResult rest =
                        reader.read_string_chunk(scratch, STRING_CHUNK_SIZE);

                    switch (rest.status) {
                    case JsonFusion::reader::StringChunkStatus::no_match:
                        err = JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW;
                        return false;
                    case JsonFusion::reader::StringChunkStatus::error:
                        return false;
                    case JsonFusion::reader::StringChunkStatus::ok:
                        break;
                    }

                    if (rest.done) {
                        err = JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW;
                        return false;
                    }

                    if (rest.bytes_written == 0) {
                        err = JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW;
                        return false;
                    }
                }
            } else {
                err = JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW;
                return false;
            }
        }

        // Calculate how much we can write
        const std::size_t remaining_for_validator = max_len - current_size;
        const std::size_t ask_hint = remaining_for_validator < STRING_CHUNK_SIZE 
                                     ? remaining_for_validator : STRING_CHUNK_SIZE;
        
        // Ask cursor for buffer space
        const std::size_t available = cursor.prepare_write(ask_hint);
        if (available == 0) {
            // Cursor has no more space (container capacity exhausted)
            if (read_rest_on_overflow) {
                // Need to consume rest of string
                char scratch[STRING_CHUNK_SIZE];
                for (;;) {
                    JsonFusion::reader::StringChunkResult rest =
                        reader.read_string_chunk(scratch, STRING_CHUNK_SIZE);
                    if (rest.status != JsonFusion::reader::StringChunkStatus::ok) {
                        if (rest.status == JsonFusion::reader::StringChunkStatus::no_match) {
                            err = JsonFusion::ParseError::NON_STRING_IN_STRING_STORAGE;
                        }
                        return false;
                    }
                    if (rest.done) {
                        err = JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW;
                        return false;
                    }
                    if (rest.bytes_written == 0) {
                        err = JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW;
                        return false;
                    }
                }
            }
            err = JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW;
            return false;
        }

        // Read directly into cursor's buffer
        char* write_buf = cursor.write_ptr();
        JsonFusion::reader::StringChunkResult res = reader.read_string_chunk(write_buf, available);

        switch (res.status) {
        case JsonFusion::reader::StringChunkStatus::no_match:
            err = JsonFusion::ParseError::NON_STRING_IN_STRING_STORAGE;
            return false;
        case JsonFusion::reader::StringChunkStatus::error:
            return false;
        case JsonFusion::reader::StringChunkStatus::ok:
            break;
        }

        // Defensive check
        if (res.bytes_written > available) {
            err = JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW;
            return false;
        }

        // Commit what was written
        if (res.bytes_written > 0) {
            cursor.commit(res.bytes_written);
        }

        if (res.done) {
            cursor.finalize();
            return true;
        }

        // Safety net: no progress = bail
        if (res.bytes_written == 0) {
            err = JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW;
            return false;
        }
    }

    err = JsonFusion::ParseError::FIXED_SIZE_CONTAINER_OVERFLOW;
    return false;
}


template <class Opts, class ObjT, reader::ReaderLike Reader, class CTX, class UserCtx = void>
    requires static_schema::ParsableStringLike<ObjT>
constexpr bool ParseNonNullValue(ObjT& obj, Reader & reader, CTX &ctx, UserCtx * userCtx = nullptr) {

    validators::validators_detail::validator_state<Opts, ObjT> validatorsState;

    // Query max string length from validators for early rejection
    using PropertyTag = validators::validators_detail::parsing_constraint_properties_tags::max_string_length;
    constexpr std::size_t validator_max = 
        validators::validators_detail::validator_state<Opts, ObjT>::template max_property<PropertyTag>();
    
    using Cursor = static_schema::string_write_cursor<ObjT>;
    Cursor cursor = [&]() {
        if constexpr (!std::is_same_v<UserCtx, void> && std::is_constructible_v<Cursor, ObjT&, UserCtx*>) {
            if (userCtx) {
                return Cursor(obj, userCtx);
            } else {
                return Cursor(obj);
            }
        } else {
            return Cursor{obj};
        }
    }();
    cursor.reset();
    
    const std::size_t container_max = cursor.max_capacity();
    const std::size_t effective_max = [&]() {
        if constexpr (validator_max > 0) {
            // Read validator_max + 1 to detect if string exceeds the limit
            constexpr std::size_t val_limit = validator_max + 1;
            return container_max < val_limit ? container_max : val_limit;
        } else {
            return container_max;
        }
    }();
    
    if constexpr (Cursor::single_pass) {
        // Fast path: direct buffer access, minimal abstraction overhead
        char* buf = cursor.write_ptr();
        std::size_t pos = 0;
        
        for (;;) {
            const std::size_t remaining = effective_max - pos;
            if (remaining == 0) {
                // Buffer full - consume rest and fail
                char scratch[64];
                for (;;) {
                    auto rest = reader.read_string_chunk(scratch, 64);
                    if (rest.status != reader::StringChunkStatus::ok) {
                        return ctx.withParseError(
                            rest.status == reader::StringChunkStatus::no_match 
                                ? ParseError::NON_STRING_IN_STRING_STORAGE 
                                : ParseError::READER_ERROR, reader);
                    }
                    if (rest.done) {
                        cursor.commit(pos);
                        cursor.finalize();
                        // Check validator limit
                        if constexpr (validator_max > 0) {
                            if (pos > validator_max) {
                                if (!validatorsState.template validate<validators::validators_detail::parsing_events_tags::string_parsing_finished>(
                                        obj, ctx.validationCtx(), cursor.view())) {
                                    return ctx.withSchemaError(reader);
                                }
                            }
                        }
                        return ctx.withParseError(ParseError::FIXED_SIZE_CONTAINER_OVERFLOW, reader);
                    }
                }
            }
            
            auto res = reader.read_string_chunk(buf + pos, remaining);
            
            if (res.status != reader::StringChunkStatus::ok) {
                return ctx.withParseError(
                    res.status == reader::StringChunkStatus::no_match 
                        ? ParseError::NON_STRING_IN_STRING_STORAGE 
                        : ParseError::READER_ERROR, reader);
            }
            
            pos += res.bytes_written;
            
            if (res.done) {
                cursor.commit(pos);
                cursor.finalize();
                
                // Validate final string
                if (!validatorsState.template validate<validators::validators_detail::parsing_events_tags::string_parsing_finished>(
                        obj, ctx.validationCtx(), cursor.view())) {
                    return ctx.withSchemaError(reader);
                }
                return true;
            }
            
            if (res.bytes_written == 0) {
                return ctx.withParseError(ParseError::FIXED_SIZE_CONTAINER_OVERFLOW, reader);
            }
        }
    } else {
        // Streaming path: use cursor abstraction for buffered writes
        ParseError err{ParseError::NO_ERROR};
        if (!read_string_with_cursor(reader, cursor, effective_max, err)) {
            const std::size_t parsedSize = cursor.size();
            
            // Check if we read enough to detect validator limit exceeded
            if (err == ParseError::FIXED_SIZE_CONTAINER_OVERFLOW && validator_max > 0 && parsedSize > validator_max) {
                // Early rejection: string exceeded validator limit
                cursor.finalize();
                if (!validatorsState.template validate<validators::validators_detail::parsing_events_tags::string_parsing_finished>(
                        obj, ctx.validationCtx(), cursor.view())) {
                    return ctx.withSchemaError(reader);
                }
            }
            return ctx.withParseError(err, reader);
        }

        // Validate final string
        if (!validatorsState.template validate<validators::validators_detail::parsing_events_tags::string_parsing_finished>(
                obj, ctx.validationCtx(), cursor.view())) {
            return ctx.withSchemaError(reader);
        }

        return true;
    }
}


template <class Opts, class ObjT, reader::ReaderLike Reader, class CTX, class UserCtx = void>
    requires static_schema::ParsableArrayLike<ObjT>
constexpr bool ParseNonNullValue(ObjT& obj, Reader & reader, CTX &ctx, UserCtx * userCtx = nullptr) {

    typename Reader::ArrayFrame fr;
    reader::IterationStatus iterStatus = reader.read_array_begin(fr);
    if(iterStatus.status == reader::TryParseStatus::no_match) {
        return ctx.withParseError(ParseError::NON_ARRAY_IN_ARRAY_LIKE_VALUE, reader);
    } else if(iterStatus.status == reader::TryParseStatus::error) {
        return ctx.withReaderError(reader);
    }
    std::size_t parsed_items_count = 0;

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
    
    // Query max array items from validators for early rejection
    using PropertyTag = validators::validators_detail::parsing_constraint_properties_tags::max_array_items;
    constexpr std::size_t validator_max_items = 
        validators::validators_detail::validator_state<Opts, ObjT>::template max_property<PropertyTag>();

    while(iterStatus.has_value) {
        // Early rejection: check if we're about to exceed validator limit
        if constexpr (validator_max_items > 0) {
            if (parsed_items_count >= validator_max_items) {
                // We've already parsed max_items, this would be max_items+1
                // Validate with the exceeded count to trigger the error
                if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::array_item_parsed>(obj, ctx.validationCtx(), parsed_items_count + 1)) {
                    cursor.finalize(false);
                    return ctx.withSchemaError(reader);
                }
            }
        }
        
        stream_write_result alloc_r = cursor.allocate_slot();
        if(alloc_r != stream_write_result::slot_allocated) {
            if(alloc_r == stream_write_result::overflow ) {
                return ctx.withParseError(ParseError::FIXED_SIZE_CONTAINER_OVERFLOW, reader);
            } else {
                return ctx.withParseError(ParseError::DATA_CONSUMER_ERROR, reader);
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
                return ctx.withParseError(ParseError::DUPLICATE_KEY_IN_MAP, reader);

            } else {
                return ctx.withParseError(ParseError::DATA_CONSUMER_ERROR, reader);
            }
        }
        parsed_items_count ++;
        if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::array_item_parsed>(obj, ctx.validationCtx(), parsed_items_count)) {
            cursor.finalize(false);
            return ctx.withSchemaError(reader);
        }
        iterStatus = reader.advance_after_value(fr);
        if (iterStatus.status != reader::TryParseStatus::ok) {
            cursor.finalize(false);
            return ctx.withReaderError(reader);
        }
    }
    if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::array_parsing_finished>(obj, ctx.validationCtx(), parsed_items_count)) {
        cursor.finalize(false);
        return ctx.withSchemaError(reader);
    }
    cursor.finalize(true);
    return true;
}


template <class Opts, class ObjT, reader::ReaderLike Reader, class CTX, class UserCtx = void>
    requires static_schema::ParsableMapLike<ObjT>
constexpr bool ParseNonNullValue(ObjT& obj, Reader & reader, CTX &ctx, UserCtx * userCtx = nullptr) {

    typename Reader::MapFrame fr;

    reader::IterationStatus iterStatus = reader.read_map_begin(fr);
    if(iterStatus.status == reader::TryParseStatus::no_match) {
        return ctx.withParseError(ParseError::NON_MAP_IN_MAP_LIKE_VALUE, reader);
    } else if(iterStatus.status == reader::TryParseStatus::error) {
        return ctx.withReaderError(reader);
    }

    std::size_t parsed_entries_count = 0;

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
    
    // Query max properties from validators for early rejection
    using PropertyTagProps = validators::validators_detail::parsing_constraint_properties_tags::max_map_properties;
    constexpr std::size_t validator_max_properties = 
        validators::validators_detail::validator_state<Opts, ObjT>::template max_property<PropertyTagProps>();

    while(iterStatus.has_value) {
        // Early rejection: check if we're about to exceed validator limit for properties count
        if constexpr (validator_max_properties > 0) {
            if (parsed_entries_count >= validator_max_properties) {
                // We've already parsed max_properties, this would be max_properties+1
                // Validate with the exceeded count to trigger the error
                if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::map_entry_parsed>(obj, ctx.validationCtx(), parsed_entries_count + 1)) {
                    cursor.finalize(false);
                    return ctx.withSchemaError(reader);
                }
            }
        }
        
        // Allocate key slot
        stream_write_result alloc_r = cursor.allocate_key();
        if(alloc_r != stream_write_result::slot_allocated) {
            if(alloc_r == stream_write_result::overflow) {
                return ctx.withParseError(ParseError::FIXED_SIZE_CONTAINER_OVERFLOW, reader);
            } else {
                return ctx.withParseError(ParseError::DATA_CONSUMER_ERROR, reader);
            }
        }

        // Parse key as string with incremental validation
        typename FH::key_type& key = cursor.key_ref();
        std::string_view key_sv;
        if constexpr(!std::integral<typename FH::key_type>) {
            // Use string_write_cursor for key parsing
            using KeyCursor = static_schema::string_write_cursor<typename FH::key_type>;
            KeyCursor keyCursor{key};
            keyCursor.reset();

            // Query max key length from validators for early rejection
            using PropertyTagKeyLen = validators::validators_detail::parsing_constraint_properties_tags::max_map_key_length;
            constexpr std::size_t validator_max_key_len = 
                validators::validators_detail::validator_state<Opts, ObjT>::template max_property<PropertyTagKeyLen>();
            
            // Calculate effective max: min of container capacity and validator limit (+1 to detect overflow)
            const std::size_t container_max = keyCursor.max_capacity();
            const std::size_t effective_max = [&]() {
                if constexpr (validator_max_key_len > 0) {
                    std::size_t val_limit = validator_max_key_len + 1;
                    return container_max < val_limit ? container_max : val_limit;
                } else {
                    return container_max;
                }
            }();

            ParseError err{ParseError::NO_ERROR};
            if(!read_string_with_cursor(reader, keyCursor, effective_max, err)) {
                const std::size_t parsedSize = keyCursor.size();
                
                // Check if we read enough to detect validator limit exceeded
                if (err == ParseError::FIXED_SIZE_CONTAINER_OVERFLOW && validator_max_key_len > 0 && parsedSize > validator_max_key_len) {
                    // Early rejection: key exceeded validator limit
                    keyCursor.finalize();
                    if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::map_key_finished>(obj, ctx.validationCtx(), keyCursor.view())) {
                        return ctx.withSchemaError(reader);
                    }
                }
                return ctx.withParseError(err, reader);
            }
            keyCursor.finalize();

            // Get key view for validation and path tracking
            key_sv = keyCursor.view();
            
            // Emit key finished event
            if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::map_key_finished>(obj, ctx.validationCtx(), key_sv)) {
                return ctx.withSchemaError(reader);
            }

        } else {
            if(!reader.read_key_as_index(key)) {
                cursor.finalize(false);
                return ctx.withReaderError(reader);
            }

            if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::map_key_finished>(obj, ctx.validationCtx(), key)) {
                cursor.finalize(false);
                return ctx.withSchemaError(reader);
            }
            key_sv = std::string_view(reinterpret_cast<char*>(&key), sizeof(key)); // TODO
        }


        if (!reader.move_to_value(fr)) {
            cursor.finalize(false);
            return ctx.withReaderError(reader);
        }

        // Allocate value slot
        alloc_r = cursor.allocate_value_for_parsed_key();
        if(alloc_r != stream_write_result::slot_allocated) {
            if(alloc_r == stream_write_result::overflow) {
                return ctx.withParseError(ParseError::FIXED_SIZE_CONTAINER_OVERFLOW, reader);
            } else {
                return ctx.withParseError(ParseError::DATA_CONSUMER_ERROR, reader);
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
            return ctx.withSchemaError(reader);
        }

        // Finalize the key-value pair
        stream_write_result finalize_r = cursor.finalize_pair(true);
        if(finalize_r != stream_write_result::value_processed) {
            if(finalize_r == stream_write_result::error) {
                return ctx.withParseError(ParseError::DUPLICATE_KEY_IN_MAP, reader);
            } else {
                return ctx.withParseError(ParseError::DATA_CONSUMER_ERROR, reader);
            }
        }

        parsed_entries_count++;

        if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::map_entry_parsed>(obj, ctx.validationCtx(), parsed_entries_count)) {
            cursor.finalize(false);
            return ctx.withSchemaError(reader);
        }
        iterStatus = reader.advance_after_value(fr);
        if (iterStatus.status != reader::TryParseStatus::ok) {
            cursor.finalize(false);
            return ctx.withReaderError(reader);
        }
    }

    if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::map_parsing_finished>(obj, ctx.validationCtx(), parsed_entries_count)) {
        cursor.finalize(false);
        return ctx.withSchemaError(reader);
    }
    cursor.finalize(true);
    return true;
}



template<class StructT, std::size_t StructIndex>
using StructFieldMeta = options::detail::annotation_meta_getter<
    introspection::structureElementTypeByIndex<StructIndex, StructT>
>;
template <class ObjT, reader::ReaderLike Reader, class CTX, class UserCtx, std::size_t... StructIndex>
    requires static_schema::ObjectLike<ObjT>
constexpr bool ParseStructField(ObjT& structObj, Reader & reader, CTX &ctx, std::index_sequence<StructIndex...>, std::size_t requiredIndex, UserCtx * userCtx = nullptr) {
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

template<std::size_t I, class ObjT, class Reader, class CTX, class UserCtx>
constexpr bool parse_struct_field_one(
    ObjT& structObj,
    Reader& reader,
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


template <class Opts, class ObjT, reader::ReaderLike Reader, class CTX, class UserCtx = void>
    requires static_schema::ObjectLike<ObjT>
constexpr bool ParseNonNullValue(ObjT& obj, Reader & reader, CTX &ctx, UserCtx * userCtx = nullptr) {

    typename Reader::MapFrame fr;

    reader::IterationStatus iterStatus = reader.read_map_begin(fr);
    if(iterStatus.status == reader::TryParseStatus::no_match) {
        return ctx.withParseError(ParseError::NON_MAP_IN_MAP_LIKE_VALUE, reader);
    } else if(iterStatus.status == reader::TryParseStatus::error) {
        return ctx.withReaderError(reader);
    }

    using FH = struct_fields_helper::FieldsHelper<ObjT>;
    static_assert(FH::fieldsAreUnique, "[[[ JsonFusion ]]] Fields keys or numeric keys are not unique");

    std::bitset<FH::fieldsCount> parsedFieldsByIndex{};
    validators::validators_detail::validator_state<Opts, ObjT> validatorsState;


    while(iterStatus.has_value) {

        std::size_t structIndex = std::size_t(-1);
        std::size_t arrayIndex = std::size_t(-1);
        std::string_view key_sv;

        if constexpr(Opts::template has_option<options::detail::indexes_as_keys_tag> || FH::hasIntegerKeys) {
            std::size_t index_to_find = 0;
            if(!reader.read_key_as_index(index_to_find)) {
                return ctx.withReaderError(reader);
            }

             // TODO
            for(arrayIndex = 0; arrayIndex < FH::fieldIndexes.size(); arrayIndex ++) {
                auto p = FH::fieldIndexes[arrayIndex];
                if(p.first == index_to_find) {
                    structIndex = p.second;
                    key_sv = std::string_view(reinterpret_cast<char*>(&arrayIndex), sizeof(arrayIndex));
                    break;
                }
            }
            if(structIndex == std::size_t(-1)) {
                arrayIndex = -1;
            }

        } else {
            // Calculate the max field name length needed for validators (e.g., forbidden fields)
            using PropertyTag = validators::validators_detail::parsing_constraint_properties_tags::max_excess_field_name_length;
            constexpr std::size_t max_validator_field_len = 
                validators::validators_detail::validator_state<Opts, ObjT>::template max_property<PropertyTag>();
            
            // Use the larger of struct field names or validator requirements
            constexpr std::size_t effective_max_field_len = 
                std::max(FH::maxFieldNameLength, max_validator_field_len);
            
            string_search::AdaptiveStringSearch<effective_max_field_len> searcher{
                FH::fieldIndexesToFieldNames.data(), FH::fieldIndexesToFieldNames.data() + FH::fieldIndexesToFieldNames.size()
            };
            char * b = searcher.buffer();

            // Create a simple raw buffer cursor wrapper for the searcher's buffer
            struct RawBufferCursor {
                char* buf;
                std::size_t& len;
                std::size_t max_cap;
                
                constexpr std::size_t prepare_write(std::size_t hint) {
                    std::size_t remaining = max_cap - len;
                    return hint < remaining ? hint : remaining;
                }
                constexpr char* write_ptr() { return buf + len; }
                constexpr void commit(std::size_t n) { len += n; }
                constexpr void reset() { len = 0; }
                constexpr void finalize() { if (len < max_cap) buf[len] = '\0'; }
                constexpr std::size_t size() const { return len; }
                constexpr std::size_t max_capacity() const { return max_cap; }
            };
            
            RawBufferCursor bufCursor{b, searcher.current_length(), effective_max_field_len};
            ParseError err{ParseError::NO_ERROR};
            if(!read_string_with_cursor(reader, bufCursor, effective_max_field_len, err, true)) {
                if(err == ParseError::NON_STRING_IN_STRING_STORAGE) {
                    return ctx.withParseError(err, reader);
                } else if (err == ParseError::FIXED_SIZE_CONTAINER_OVERFLOW) {
                    searcher.set_overflow();
                } else {
                    return ctx.withReaderError(reader);
                }
            }
            auto res = searcher.result();
            if( searcher.result() != FH::fieldIndexesToFieldNames.end()) {
                arrayIndex = res - FH::fieldIndexesToFieldNames.begin();
                structIndex = res->originalIndex;
                key_sv = res->name;
            } else {
                // Field not found in struct - it's an excess field
                // Validate forbidden fields before searcher goes out of scope
                if(arrayIndex == std::size_t(-1)) {
                    std::string_view excess_field_name(b, searcher.current_length());
                    if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::excess_field_occured>(obj, ctx.validationCtx(), excess_field_name, FH{})) {
                        if (!reader.move_to_value(fr)) {
                            return ctx.withReaderError(reader);
                        }
                        return ctx.withSchemaError(reader);
                    }
                }
            }

        }
        if (!reader.move_to_value(fr)) {
            return ctx.withReaderError(reader);
        }

        if(arrayIndex == std::size_t(-1)) {
            if constexpr (Opts::template has_option<options::detail::allow_excess_fields_tag>) {
                if(!reader.skip_value()) {
                    return ctx.withReaderError(reader);
                }
            } else {
                return ctx.withParseError(ParseError::EXCESS_FIELD, reader);
            }
        } else {
            if(parsedFieldsByIndex[arrayIndex] == true) {
                return ctx.withParseError(ParseError::DUPLICATE_KEY_IN_MAP, reader);
            }

            typename CTX::PathGuard guard = ctx.getMapItemGuard(key_sv);

            if(!ParseStructField(obj, reader, ctx, std::make_index_sequence<FH::rawFieldsCount>{}, structIndex, userCtx)) {
                return false;
            }

            parsedFieldsByIndex[arrayIndex] = true;
            if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::object_field_parsed>(obj, ctx.validationCtx(), arrayIndex, FH{})) {
                return ctx.withSchemaError(reader);
            }
        }

        iterStatus = reader.advance_after_value(fr);
        if (iterStatus.status != reader::TryParseStatus::ok) {
            return ctx.withReaderError(reader);
        }
    }
    if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::object_parsing_finished>(obj, ctx.validationCtx(), parsedFieldsByIndex, FH{})) {
        return ctx.withSchemaError(reader);
    }

    return true;
}


/* #### SPECIAL CASE FOR ARRAYS DESRTUCTURING #### */





template <class Opts, class ObjT, reader::ReaderLike Reader, class CTX, class UserCtx = void>
    requires static_schema::ObjectLike<ObjT>
             &&
             Opts::template has_option<options::detail::as_array_tag>
constexpr bool ParseNonNullValue(ObjT& obj, Reader & reader, CTX &ctx, UserCtx * userCtx = nullptr) {
    typename Reader::ArrayFrame fr;
    reader::IterationStatus iterStatus = reader.read_array_begin(fr);
    if(iterStatus.status == reader::TryParseStatus::no_match) {
        return ctx.withParseError(ParseError::NON_ARRAY_IN_ARRAY_LIKE_VALUE, reader);
    } else if(iterStatus.status == reader::TryParseStatus::error) {
        return ctx.withReaderError(reader);
    }

    std::size_t requiredIndex = 0;
    std::size_t parsed_items_count = 0;

    constexpr std::size_t totalFieldsCount = introspection::structureElementsCount<ObjT>;
    validators::validators_detail::validator_state<Opts, ObjT> validatorsState;


    static constexpr std::array<bool, totalFieldsCount> isFieldSkipped = []<std::size_t... I>(std::index_sequence<I...>) {
        return std::array<bool, sizeof...(I)>{
            struct_fields_helper::fieldIsNotJSON<ObjT, I>()...
        };
    }(std::make_index_sequence<totalFieldsCount>{});

    while(iterStatus.has_value) {

        while(requiredIndex < totalFieldsCount && isFieldSkipped[requiredIndex]) {
            requiredIndex ++;
        }

        if(requiredIndex >= totalFieldsCount) {
            return ctx.withParseError(ParseError::ARRAY_DESTRUCTURING_SCHEMA_ERROR, reader);
        }

        if(!ParseStructField(obj, reader, ctx, std::make_index_sequence<totalFieldsCount>{}, requiredIndex, userCtx)) {
            return false;
        }

        parsed_items_count ++;
        requiredIndex ++;

        iterStatus = reader.advance_after_value(fr);
        if (iterStatus.status != reader::TryParseStatus::ok) {
            return ctx.withReaderError(reader);
        }
    }
    while(requiredIndex < totalFieldsCount && isFieldSkipped[requiredIndex]) {
        requiredIndex ++;
    }

    if(requiredIndex!= totalFieldsCount) {
        return ctx.withParseError(ParseError::ARRAY_DESTRUCTURING_SCHEMA_ERROR, reader);
    }

    if(!validatorsState.template validate<validators::validators_detail::parsing_events_tags::descrtuctured_object_parsing_finished>(obj, ctx.validationCtx())) {
        return ctx.withSchemaError(reader);
    }

    return true;
}

template <class FieldOptions, static_schema::ParsableValue Field, reader::ReaderLike Reader, class CTX, class UserCtx = void>
    requires (!static_schema::ParseTransformerLike<Field> && !WireSinkLike<Field>)
constexpr bool ParseValue(Field & field, Reader & reader, CTX &ctx, UserCtx * userCtx = nullptr) {

    if constexpr (FieldOptions::template has_option<options::detail::exclude_tag>) {
        return false; // cannot parse non-json
    }else if constexpr (FieldOptions::template has_option<options::detail::skip_tag>) {
        if(!reader.skip_value()) {
            return ctx.withReaderError(reader);
        } else {
            return true;
        }
    } else {
        if(reader::TryParseStatus r = reader.start_value_and_try_read_null(); r == reader::TryParseStatus::ok) {
            if constexpr(static_schema::NullableParsableValue<Field>) {
                static_schema::setNull(field);
                return true;
            } else {
                return ctx.withParseError(ParseError::NULL_IN_NON_OPTIONAL, reader);
            }
        } else if(r == reader::TryParseStatus::error) {
            return ctx.withReaderError(reader);
        } else {
            return ParseNonNullValue<FieldOptions>(static_schema::getRef(field), reader, ctx, userCtx);
        }
    }
}

// WireSink: First-class wire format capture
template <class FieldOptions, static_schema::ParsableValue Field, reader::ReaderLike Reader, class CTX, class UserCtx = void>
    requires WireSinkLike<Field>
constexpr bool ParseValue(Field & field, Reader & reader, CTX &ctx, UserCtx * userCtx = nullptr) {
    field.clear();
    if (!reader.capture_to_sink(field)) {
        return ctx.withReaderError(reader);
    }
    return true;
}

template <class FieldOptions, static_schema::ParsableValue Field, reader::ReaderLike Reader, class CTX, class UserCtx = void>
    requires static_schema::ParseTransformerLike<Field>
constexpr bool ParseValue(Field & field, Reader & reader, CTX &ctx, UserCtx * userCtx = nullptr) {
    using StT = static_schema::parse_transform_traits<Field>::wire_type;
    StT ob;
    using Meta = options::detail::annotation_meta_getter<StT>;
    bool r = parser_details::ParseValue<typename Meta::options>(Meta::getRef(ob), reader, ctx, userCtx);
    if(r) {
        if constexpr(!WireSinkLike<StT>) {
            if(!field.transform_from(ob)) {
                return ctx.withParseError(ParseError::TRANSFORMER_ERROR, reader);
            }
        } else {
            auto parseFn = [&]<class ObjT>(ObjT& targetObj) constexpr {
                return ParseWithReader(targetObj, Reader::from_sink(ob), userCtx);
            };
            if(!field.transform_from(parseFn)) {
                return ctx.withParseError(ParseError::TRANSFORMER_ERROR, reader);
            }
        }
    }
    return r;
}


} // namespace parser_details


template <static_schema::ParsableValue InputObjectT, class UserCtx = void, reader::ReaderLike Reader>
constexpr auto ParseWithReader(InputObjectT & obj, Reader & reader, UserCtx * userCtx = nullptr) {
    using Tr = ModelParsingTraits<InputObjectT>;
    using CtxT = parser_details::DeserializationContext<typename Reader::iterator_type, Tr::SchemaDepth, Tr::SchemaHasMaps, typename Reader::error_type>;

    CtxT ctx;

    using Meta = options::detail::annotation_meta_getter<InputObjectT>;

    parser_details::ParseValue<typename Meta::options>(Meta::getRef(obj), reader, ctx, userCtx);

    if(ctx.currentError() == ParseError::NO_ERROR) {
        if(!reader.finish()) {
            ctx.withReaderError(reader);
        }
    }
    return ctx.result();
}

template <static_schema::ParsableValue InputObjectT, class UserCtx = void, reader::ReaderLike Reader>
constexpr auto ParseWithReader(InputObjectT & obj, Reader && reader, UserCtx * userCtx = nullptr) {
    return ParseWithReader(obj, reader, userCtx);
}

template <static_schema::ParsableValue InputObjectT, CharInputIterator It, CharSentinelFor<It> Sent, class UserCtx = void, class Reader = JsonIteratorReader<It, Sent>>
constexpr auto Parse(InputObjectT & obj, It begin, const Sent & end, UserCtx * userCtx = nullptr) {
    Reader reader(begin, end);
    return ParseWithReader(obj, reader, userCtx);
}


template<class InputObjectT, class ContainterT, class UserCtx = void>
    requires (!std::is_pointer_v<ContainterT>) && requires(const ContainterT& c) { c.begin(); c.end(); }
constexpr auto Parse(InputObjectT & obj, const ContainterT & c, UserCtx * userCtx = nullptr) {
    return Parse(obj, c.begin(), c.end(), userCtx);
}

template<static_schema::ParsableValue InputObjectT, class UserCtx = void>
constexpr auto Parse(InputObjectT& obj, std::string_view sv, UserCtx * userCtx = nullptr) {
    return Parse(obj, sv.data(), sv.data() + sv.size(), userCtx);
}

template<static_schema::ParsableValue InputObjectT, WireSinkLike Sink, class UserCtx = void>
constexpr auto Parse(InputObjectT& obj, const Sink& sink, UserCtx * userCtx = nullptr) {
    return Parse(obj, sink.data(), sink.data() + sink.current_size(), userCtx);
}


// string front-end TODO
// template<static_schema::ParsableValue InputObjectT, class UserCtx = void>
// constexpr auto Parse(InputObjectT& obj, const std::string & sv, UserCtx * userCtx = nullptr) {
//     return Parse(obj, sv.data(), sv.data()+ sv.size(), userCtx);
// }


template <class T>
    requires (!static_schema::ParsableValue<T>)
constexpr auto  Parse(T obj, auto, auto) {
    static_assert(static_schema::detail::always_false<T>::value,
                  "[[[ JsonFusion ]]] T is not a supported JsonFusion parsable value model type.\n"
                  "see ParsableValue concept for full rules");
}

template <class T>
    requires (!static_schema::ParsableValue<T>)
constexpr auto Parse(T obj, auto) {
    static_assert(static_schema::detail::always_false<T>::value,
                  "[[[ JsonFusion ]]] T is not a supported JsonFusion parsable value model type.\n"
                  "see ParsableValue concept for full rules");
}


} // namespace JsonFusion
