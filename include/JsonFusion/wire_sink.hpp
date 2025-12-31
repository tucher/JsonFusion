#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
#include <algorithm>
#include <limits>
#include <concepts>

namespace JsonFusion {

// ============================================================================
// WireSinkLike Concept
// ============================================================================

/// Concept for types that can act as wire format capture buffers.
/// Wire sinks are protocol-agnostic byte buffers that capture raw wire data
/// during parsing and emit it during serialization.
/// 
/// Different readers/writers may store different data:
/// - JSON: compact text bytes
/// - CBOR: binary CBOR bytes
/// - DOM readers: node handles/pointers
/// - Streaming: file offsets + lengths
template<typename T>
concept WireSinkLike = requires(T& sink, const T& csink, 
                                 const std::uint8_t* data, std::uint8_t* out,
                                 std::size_t size, std::size_t offset) {
    { sink.write(data, size) } -> std::same_as<bool>;
    { csink.read(out, size, offset) } -> std::same_as<bool>;
    { csink.max_size() } -> std::convertible_to<std::size_t>;
    { csink.current_size() } -> std::convertible_to<std::size_t>;
    { sink.clear() } -> std::same_as<void>;
    { csink.data() } -> std::same_as<const std::uint8_t*>;
    { sink.data() } -> std::same_as<std::uint8_t*>;
};

// ============================================================================
// WireSink - Generic Wire Format Capture Buffer
// ============================================================================

/// Primary template for wire format capture buffers.
/// 
/// Template parameters:
///   MaxSize  - Maximum buffer size in bytes (required, no default)
///   dynamic  - If true, uses dynamic allocation (std::vector)
///              If false, uses static allocation (std::array)
/// 
/// Examples:
///   WireSink<1024>          - Static 1KB buffer on stack
///   WireSink<1024, false>   - Same as above (explicit)
///   WireSink<65536, true>   - Dynamic buffer, max 64KB
/// 
/// Design rationale:
/// - No default MaxSize: forces users to think about size limits
/// - Dynamic storage still bounded: prevents unbounded memory growth
/// - Protocol-agnostic: stores bytes, reader/writer interprets them
template<std::size_t MaxSize, bool dynamic = false>
class WireSink;

// ============================================================================
// Specialization: Static Storage (std::array)
// ============================================================================

template<std::size_t MaxSize>
class WireSink<MaxSize, false> {
    std::array<std::uint8_t, MaxSize> data_{};
    std::size_t size_ = 0;
    
public:
    constexpr WireSink() = default;
    
    /// Write bytes to the end of the buffer
    /// Returns false if buffer would overflow
    constexpr bool write(const std::uint8_t* bytes, std::size_t count) {
        if (size_ + count > MaxSize) {
            return false; // Buffer overflow
        }
        std::copy_n(bytes, count, data_.begin() + size_);
        size_ += count;
        return true;
    }
    
    /// Read bytes from the buffer at given offset
    /// Returns false if read would exceed current size
    constexpr bool read(std::uint8_t* out, std::size_t count, std::size_t offset) const {
        if (offset + count > size_) {
            return false; // Read beyond current data
        }
        std::copy_n(data_.begin() + offset, count, out);
        return true;
    }
    
    /// Maximum capacity of this buffer
    constexpr std::size_t max_size() const { 
        return MaxSize; 
    }
    
    /// Current number of bytes stored
    constexpr std::size_t current_size() const { 
        return size_; 
    }
    
    /// Clear buffer (reset size to 0, does not overwrite data)
    constexpr void clear() { 
        size_ = 0; 
    }
    
    // ---- Convenience access methods ----
    
    /// Direct access to underlying buffer (read-only)
    constexpr const std::uint8_t* data() const { 
        return data_.data(); 
    }
    
    /// Direct access to underlying buffer (mutable)
    constexpr std::uint8_t* data() { 
        return data_.data(); 
    }
};

// ============================================================================
// Specialization: Dynamic Storage (std::vector)
// ============================================================================

template<std::size_t MaxSize>
class WireSink<MaxSize, true> {
    std::vector<std::uint8_t> data_;
    
public:
    constexpr WireSink() = default;
    
    /// Write bytes to the end of the buffer
    /// Returns false if adding bytes would exceed MaxSize limit
    constexpr bool write(const std::uint8_t* bytes, std::size_t count) {
        if (data_.size() + count > MaxSize) {
            return false; // Would exceed maximum size
        }
        data_.insert(data_.end(), bytes, bytes + count);
        return true;
    }
    
    /// Read bytes from the buffer at given offset
    /// Returns false if read would exceed current size
    constexpr bool read(std::uint8_t* out, std::size_t count, std::size_t offset) const {
        if (offset + count > data_.size()) {
            return false; // Read beyond current data
        }
        std::copy_n(data_.begin() + offset, count, out);
        return true;
    }
    
    /// Maximum capacity of this buffer
    constexpr std::size_t max_size() const { 
        return MaxSize; 
    }
    
    /// Current number of bytes stored
    constexpr std::size_t current_size() const { 
        return data_.size(); 
    }
    
    /// Clear buffer (deallocates memory)
    constexpr void clear() { 
        data_.clear(); 
    }
    
    // ---- Convenience access methods ----
    
    /// Direct access to underlying buffer (read-only)
    constexpr const std::uint8_t* data() const { 
        return data_.data(); 
    }
    
    /// Direct access to underlying buffer (mutable)
    constexpr std::uint8_t* data() { 
        return data_.data(); 
    }
};

// ============================================================================
// Static Assertions - Verify Concept Satisfaction
// ============================================================================

static_assert(WireSinkLike<WireSink<256>>, "WireSink<256> should satisfy WireSinkLike");
static_assert(WireSinkLike<WireSink<1024>>, "WireSink<1024> should satisfy WireSinkLike");
static_assert(WireSinkLike<WireSink<65536, true>>, "WireSink<65536, true> should satisfy WireSinkLike");

} // namespace JsonFusion

