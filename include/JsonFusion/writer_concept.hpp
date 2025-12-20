#pragma once

#include <concepts>

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
    { mutable_writer.advance_after_value(arrFrameRef) } -> std::same_as<bool>;
    { mutable_writer.advance_after_value(mapFrameRef) } -> std::same_as<bool>;
    { mutable_writer.move_to_value(mapFrameRef) } -> std::same_as<bool>;
    { mutable_writer.write_key_as_index(sizeRef) } -> std::same_as<bool>;

    { mutable_writer.write_array_end(arrFrameRef) } -> std::same_as<bool>;
    { mutable_writer.write_map_end(mapFrameRef) } -> std::same_as<bool>;

    // ========== Primitive Value Parsing ==========
    // Null parsing
    { mutable_writer.write_null() } -> std::same_as<bool>;
    
    // Boolean parsing
    { mutable_writer.write_bool(bool_ref) } -> std::same_as<bool>;
    
    { mutable_writer.template write_number<int>(int_ref) } -> std::same_as<bool>;
    { mutable_writer.template write_number<double>(double_ref) } -> std::same_as<bool>;
    
    { mutable_writer.write_string(char_ptr, size, false) } -> std::same_as<bool>;
    


    // ========== Utility Operations ==========
    // Skip to end, ensuring only whitespace remains
    { mutable_writer.finish() } -> std::same_as<bool>;
    
  };

/// Type trait to check if a type satisfies ReaderLike at compile time
template<typename R>
constexpr bool is_writer_like_v = WriterLike<R>;


} // namespace reader

}
