#pragma once

#include <concepts>
#include <limits>
#include "parse_errors.hpp"

namespace JsonFusion {

namespace json_reader {
enum class TryParseStatus {
    no_match,   // not our case, iterator unchanged
    ok,         // parsed and consumed
    error       // malformed, ctx already has error
};

enum class StringChunkStatus {
    ok,       // wrote some bytes (maybe zero), no error
    no_match, // not at a string (no '"' at start, and not already in_string_)
    error     // parse error set in ctx
};

struct StringChunkResult {
    StringChunkStatus status;
    std::size_t       bytes_written; // how many bytes we put into `out`
    bool              done;          // true if closing '"' was consumed
};



/// ReaderLike concept defines the interface that any JSON reader implementation must satisfy
/// This allows for alternative reader implementations (e.g., SIMD-based, memory-mapped, etc.)
/// while maintaining compatibility with the parser infrastructure.
template<typename R>
concept ReaderLike = requires(R reader, 
                               R& mutable_reader,
                               bool& bool_ref, 
                               int& int_ref,
                               double& double_ref,
                               char* char_ptr,
                               std::size_t size) {
    
    // ========== Type Requirements ==========
    // Reader must expose its iterator type
    typename R::iterator_type;
    
    // ========== Iterator Access ==========
    // Provides access to current position
    { reader.current() } -> std::same_as<typename R::iterator_type>;
    
    // ========== Error Handling ==========
    // Returns the current parse error state
    { reader.getError() } -> std::same_as<ParseError>;
    
    // ========== Structural Tokens ==========
    // Array parsing
    { mutable_reader.read_array_begin() } -> std::same_as<bool>;
    { mutable_reader.read_array_end() } -> std::same_as<TryParseStatus>;
    
    // Object parsing
    { mutable_reader.read_object_begin() } -> std::same_as<bool>;
    { mutable_reader.read_object_end() } -> std::same_as<TryParseStatus>;
    
    // Separators
    { mutable_reader.consume_value_separator(bool_ref) } -> std::same_as<bool>;
    { mutable_reader.consume_kv_separator() } -> std::same_as<bool>;
    
    // ========== Primitive Value Parsing ==========
    // Null parsing (with whitespace skipping)
    { mutable_reader.skip_ws_and_read_null() } -> std::same_as<TryParseStatus>;
    
    // Boolean parsing
    { mutable_reader.read_bool(bool_ref) } -> std::same_as<TryParseStatus>;
    
    // Number parsing (with optional materialization skip)
    { mutable_reader.template read_number<int, false>(int_ref) } -> std::same_as<TryParseStatus>;
    { mutable_reader.template read_number<double, false>(double_ref) } -> std::same_as<TryParseStatus>;
    { mutable_reader.template read_number<int, true>(int_ref) } -> std::same_as<TryParseStatus>;
    
    // String parsing (chunked, for streaming)
    { mutable_reader.read_string_chunk(char_ptr, size) } -> std::same_as<StringChunkResult>;
    
    // ========== Utility Operations ==========
    // Skip to end, ensuring only whitespace remains
    { mutable_reader.skip_whitespaces_till_the_end() } -> std::same_as<bool>;
    
    // Skip entire JSON value (with optional output to sink)
    { mutable_reader.template skip_json_value<2>(static_cast<void*>(nullptr), std::numeric_limits<std::size_t>::max()) } -> std::same_as<bool>;
};

/// Type trait to check if a type satisfies ReaderLike at compile time
template<typename R>
constexpr bool is_reader_like_v = ReaderLike<R>;


} // namespace JsonFusion

}
