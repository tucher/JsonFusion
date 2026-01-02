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

/// Cleanup callback type for protocol-specific resource management.
/// The callback receives the buffer data and size, allowing it to:
/// - Decode pointers/handles stored in the buffer
/// - Free associated resources (e.g., DOM documents)
/// - Perform any other cleanup needed
/// 
/// constexpr-compatible via function pointer (not std::function).
using WireSinkCleanupFn = void(*)(char* data, std::size_t size);

/// Concept for types that can act as wire format capture buffers.
/// Wire sinks are protocol-agnostic byte buffers that capture raw wire data
/// during parsing and emit it during serialization.
/// 
/// Different readers/writers may store different data:
/// - JSON: compact text bytes (no cleanup needed)
/// - CBOR: binary CBOR bytes (no cleanup needed)
/// - DOM readers: node handles/pointers + document ownership (cleanup frees doc)
/// - Streaming: file offsets + lengths (no cleanup needed)
/// 
/// Storage is `char` for constexpr compatibility and direct string_view conversion.
/// Binary protocols can still use char storage - bytes are bytes.
/// 
/// Lifecycle: WireSink owns its data and calls cleanup (if set) on destruction,
/// ensuring RAII semantics for any protocol-specific resources.
template<typename T>
concept WireSinkLike = requires(T& sink, const T& csink, 
                                 const char* data, char* out,
                                 std::size_t size, std::size_t offset,
                                 WireSinkCleanupFn cleanup) {
    { sink.write(data, size) } -> std::same_as<bool>;
    { csink.read(out, size, offset) } -> std::same_as<bool>;
    { csink.max_size() } -> std::convertible_to<std::size_t>;
    { csink.current_size() } -> std::convertible_to<std::size_t>;
    { sink.clear() } -> std::same_as<void>;
    { sink.set_size(size) } -> std::same_as<bool>;
    { csink.data() } -> std::same_as<const char*>;
    { sink.data() } -> std::same_as<char*>;
    { sink.set_cleanup(cleanup) } -> std::same_as<void>;
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
    std::array<char, MaxSize> data_{};
    std::size_t size_ = 0;
    WireSinkCleanupFn cleanup_ = nullptr;
    
public:
    constexpr WireSink() = default;
    
    /// Destructor: calls cleanup if set (for protocol-specific resource management)
    constexpr ~WireSink() {
        if (cleanup_) {
            cleanup_(data_.data(), size_);
        }
    }
    
    // Non-copyable (owns resources), movable
    WireSink(const WireSink&) = delete;
    WireSink& operator=(const WireSink&) = delete;
    
    constexpr WireSink(WireSink&& other) noexcept 
        : data_(std::move(other.data_))
        , size_(other.size_)
        , cleanup_(other.cleanup_)
    {
        other.size_ = 0;
        other.cleanup_ = nullptr;  // Transfer ownership
    }
    
    constexpr WireSink& operator=(WireSink&& other) noexcept {
        if (this != &other) {
            // Clean up current resources
            if (cleanup_) {
                cleanup_(data_.data(), size_);
            }
            // Transfer from other
            data_ = std::move(other.data_);
            size_ = other.size_;
            cleanup_ = other.cleanup_;
            other.size_ = 0;
            other.cleanup_ = nullptr;
        }
        return *this;
    }
    
    /// Write bytes to the end of the buffer
    /// Returns false if buffer would overflow
    constexpr bool write(const char* bytes, std::size_t count) {
        if (size_ + count > MaxSize) {
            return false; // Buffer overflow
        }
        std::copy_n(bytes, count, data_.begin() + size_);
        size_ += count;
        return true;
    }
    
    /// Read bytes from the buffer at given offset
    /// Returns false if read would exceed current size
    constexpr bool read(char* out, std::size_t count, std::size_t offset) const {
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
        // Run cleanup before clearing (if set)
        if (cleanup_) {
            cleanup_(data_.data(), size_);
            cleanup_ = nullptr;  // Don't run again
        }
        size_ = 0; 
    }
    
    /// Set cleanup callback for protocol-specific resource management
    /// The callback will be called on destruction or clear()
    constexpr void set_cleanup(WireSinkCleanupFn fn) {
        cleanup_ = fn;
    }
    
    /// Set the current size (for direct buffer manipulation via data())
    /// Use this after writing directly to data() pointer to update the tracked size
    /// Returns false if new_size exceeds MaxSize
    constexpr bool set_size(std::size_t new_size) {
        if (new_size > MaxSize) return false;
        size_ = new_size;
        return true;
    }
    
    // ---- Convenience access methods ----
    
    /// Direct access to underlying buffer (read-only)
    constexpr const char* data() const { 
        return data_.data(); 
    }
    
    /// Direct access to underlying buffer (mutable)
    constexpr char* data() { 
        return data_.data(); 
    }
};

// ============================================================================
// Specialization: Dynamic Storage (std::vector)
// ============================================================================

template<std::size_t MaxSize>
class WireSink<MaxSize, true> {
    std::vector<char> data_;
    WireSinkCleanupFn cleanup_ = nullptr;
    
public:
    constexpr WireSink() = default;
    
    /// Destructor: calls cleanup if set (for protocol-specific resource management)
    constexpr ~WireSink() {
        if (cleanup_) {
            cleanup_(data_.data(), data_.size());
        }
    }
    
    // Non-copyable (owns resources), movable
    WireSink(const WireSink&) = delete;
    WireSink& operator=(const WireSink&) = delete;
    
    constexpr WireSink(WireSink&& other) noexcept 
        : data_(std::move(other.data_))
        , cleanup_(other.cleanup_)
    {
        other.cleanup_ = nullptr;  // Transfer ownership
    }
    
    constexpr WireSink& operator=(WireSink&& other) noexcept {
        if (this != &other) {
            // Clean up current resources
            if (cleanup_) {
                cleanup_(data_.data(), data_.size());
            }
            // Transfer from other
            data_ = std::move(other.data_);
            cleanup_ = other.cleanup_;
            other.cleanup_ = nullptr;
        }
        return *this;
    }
    
    /// Write bytes to the end of the buffer
    /// Returns false if adding bytes would exceed MaxSize limit
    constexpr bool write(const char* bytes, std::size_t count) {
        if (data_.size() + count > MaxSize) {
            return false; // Would exceed maximum size
        }
        data_.insert(data_.end(), bytes, bytes + count);
        return true;
    }
    
    /// Read bytes from the buffer at given offset
    /// Returns false if read would exceed current size
    constexpr bool read(char* out, std::size_t count, std::size_t offset) const {
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
        // Run cleanup before clearing (if set)
        if (cleanup_) {
            cleanup_(data_.data(), data_.size());
            cleanup_ = nullptr;  // Don't run again
        }
        data_.clear(); 
    }
    
    /// Set cleanup callback for protocol-specific resource management
    /// The callback will be called on destruction or clear()
    constexpr void set_cleanup(WireSinkCleanupFn fn) {
        cleanup_ = fn;
    }
    
    /// Set the current size (for direct buffer manipulation via data())
    /// Use this after writing directly to data() pointer to update the tracked size
    /// Returns false if new_size exceeds MaxSize or requires reallocation that would exceed MaxSize
    constexpr bool set_size(std::size_t new_size) {
        if (new_size > MaxSize) return false;
        data_.resize(new_size);
        return true;
    }
    
    // ---- Convenience access methods ----
    
    /// Direct access to underlying buffer (read-only)
    constexpr const char* data() const { 
        return data_.data(); 
    }
    
    /// Direct access to underlying buffer (mutable)
    constexpr char* data() { 
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

