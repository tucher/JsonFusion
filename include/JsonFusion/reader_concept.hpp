#pragma once

#include <concepts>
#include <limits>

namespace JsonFusion {

namespace reader {
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

struct IterationStatus {
    TryParseStatus status = TryParseStatus::error;
    bool has_value = false; 
};


/// ReaderLike concept defines the interface that any format reader implementation must satisfy
/// This allows for alternative reader implementations (e.g., SIMD-based, memory-mapped, etc.)
/// while maintaining compatibility with the parser infrastructure.
template<typename R>
concept ReaderLike = requires(R reader, 
                               R& mutable_reader,
                               bool& bool_ref, 
                               int& int_ref,
                               double& double_ref,
                               char* char_ptr,
                               std::size_t size,
                               std::size_t & sizeRef,
                               typename R::ArrayFrame & arrFrameRef,
                               typename R::MapFrame & mapFrameRef
                              ) {
    
    // ========== Type Requirements ==========
    // Reader must expose its iterator type
    typename R::iterator_type;
    typename R::ArrayFrame;
    typename R::MapFrame;
    typename R::error_type;
    
    // ========== Iterator Access ==========
    // Provides access to current position
    { reader.current() } -> std::same_as<typename R::iterator_type>;
    
    // Returns the current parse error state
    { reader.getError() } -> std::same_as<typename R::error_type>;
    
    { mutable_reader.read_array_begin(arrFrameRef) } -> std::same_as<IterationStatus>;
    { mutable_reader.read_map_begin(mapFrameRef) } -> std::same_as<IterationStatus>;
    
    // Containers
    { mutable_reader.advance_after_value(arrFrameRef) } -> std::same_as<IterationStatus>;
    { mutable_reader.advance_after_value(mapFrameRef) } -> std::same_as<IterationStatus>;
    { mutable_reader.move_to_value(mapFrameRef) } -> std::same_as<bool>;
    { mutable_reader.read_key_as_index(sizeRef) } -> std::same_as<bool>;

    // ========== Primitive Value Parsing ==========
    // Null parsing
    { mutable_reader.start_value_and_try_read_null() } -> std::same_as<TryParseStatus>;
    
    // Boolean parsing
    { mutable_reader.read_bool(bool_ref) } -> std::same_as<TryParseStatus>;
    
    { mutable_reader.template read_number<int>(int_ref) } -> std::same_as<TryParseStatus>;
    { mutable_reader.template read_number<double>(double_ref) } -> std::same_as<TryParseStatus>;
    
    // String parsing (chunked, for streaming)
    { mutable_reader.read_string_chunk(char_ptr, size) } -> std::same_as<StringChunkResult>;
    


    // ========== Utility Operations ==========
    // Skip to end, ensuring only whitespace remains
    { mutable_reader.finish() } -> std::same_as<bool>;
    
    // Skip entire value (with optional output to sink)
    { mutable_reader.template skip_value<2>(static_cast<void*>(nullptr), std::numeric_limits<std::size_t>::max()) } -> std::same_as<bool>;
};

/// Type trait to check if a type satisfies ReaderLike at compile time
template<typename R>
constexpr bool is_reader_like_v = ReaderLike<R>;


} // namespace reader

}
