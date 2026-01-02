#pragma once

#include <concepts>
#include "wire_sink.hpp"

namespace JsonFusion {

namespace writer {

template<typename R>
concept WriterLike = requires(R writer, 
                               R& mutable_writer,
                               const bool& bool_ref, 
                               const int& int_ref,
                               const double& double_ref,
                               const char* char_ptr,
                               std::size_t size,
                               const std::size_t & sizeRef,
                               typename R::ArrayFrame & arrFrameRef,
                               typename R::MapFrame & mapFrameRef
                              ) {
    
    // ========== Type Requirements ==========
    // Writer must expose its iterator type
    typename R::iterator_type;
    typename R::ArrayFrame;
    typename R::MapFrame;
    typename R::error_type;

    // ========== Iterator Access ==========
    // Provides access to current position
    { writer.current() } -> std::same_as<typename R::iterator_type &>;
    
    // Returns the current writing error state
    { writer.getError() } -> std::same_as<typename R::error_type>;
    
    { mutable_writer.write_array_begin(sizeRef, arrFrameRef) } -> std::same_as<bool>;
    { mutable_writer.write_map_begin(sizeRef, mapFrameRef) } -> std::same_as<bool>;
    
    // Containers

    // IMPORTANT: Call this BETWEEN elements, not after the each element.
    /// Think of it as "write a separator before the next value."
    { mutable_writer.advance_after_value(arrFrameRef) } -> std::same_as<bool>;
    { mutable_writer.advance_after_value(mapFrameRef) } -> std::same_as<bool>;
    { mutable_writer.move_to_value(mapFrameRef) } -> std::same_as<bool>;
    { mutable_writer.write_key_as_index(sizeRef) } -> std::same_as<bool>;

    { mutable_writer.write_array_end(arrFrameRef) } -> std::same_as<bool>;
    { mutable_writer.write_map_end(mapFrameRef) } -> std::same_as<bool>;

    // ========== Primitive Value Writing ==========
    // Null writing
    { mutable_writer.write_null() } -> std::same_as<bool>;
    
    // Boolean writing
    { mutable_writer.write_bool(bool_ref) } -> std::same_as<bool>;
    
    { mutable_writer.template write_number<int>(int_ref) } -> std::same_as<bool>;
    { mutable_writer.template write_number<double>(double_ref) } -> std::same_as<bool>;
    
    // String writing (data pointer, size, null_terminated flag)
    { mutable_writer.write_string(char_ptr, size, false) } -> std::same_as<bool>;
    


    // ========== Utility Operations ==========
    // Skip to end, ensuring only whitespace remains
    { mutable_writer.finish() } -> std::same_as<bool>;
    
    // ========== WireSink Operations ==========
    // Output wire sink contents as a value (protocol-agnostic raw data emission)
    { mutable_writer.output_from_sink(std::declval<const WireSink<256>&>()) } -> std::same_as<bool>;
    { mutable_writer.output_from_sink(std::declval<const WireSink<1024, true>&>()) } -> std::same_as<bool>;
    
    // Create a writer from a WireSink (for serializing to captured buffer)
    // Static method: W::from_sink(iterator, sink) -> W
    { R::from_sink(std::declval<char*&>(), std::declval<WireSink<256>&>()) };
  };

/// Type trait to check if a type satisfies WriterLike at compile time
template<typename R>
constexpr bool is_writer_like_v = WriterLike<R>;


} // namespace writer

}
