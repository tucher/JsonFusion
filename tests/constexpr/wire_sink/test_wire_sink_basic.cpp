#include "../test_helpers.hpp"
#include <JsonFusion/wire_sink.hpp>
#include <string>
#include <cstring>

using namespace JsonFusion;
using namespace TestHelpers;

// ============================================================================
// Test: WireSink Basic Functionality
// ============================================================================

// Test: Static WireSink - write and read
constexpr bool test_static_wire_sink_write_read() {
    WireSink<256> sink;
    
    // Check initial state
    if (sink.current_size() != 0) return false;
    if (sink.max_size() != 256) return false;
    
    // Write some data
    const std::uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    if (!sink.write(data, 5)) return false;
    
    if (sink.current_size() != 5) return false;
    
    // Read it back
    std::uint8_t buffer[5] = {};
    if (!sink.read(buffer, 5, 0)) return false;
    
    // Verify
    for (std::size_t i = 0; i < 5; ++i) {
        if (buffer[i] != data[i]) return false;
    }
    
    return true;
}
static_assert(test_static_wire_sink_write_read(), "Static WireSink write/read failed");

// Test: Static WireSink - overflow protection
constexpr bool test_static_wire_sink_overflow() {
    WireSink<10> sink;  // Small buffer
    
    const std::uint8_t data[20] = {};
    
    // Writing 20 bytes to 10-byte buffer should fail
    if (sink.write(data, 20)) return false;
    
    // Writing 10 bytes should succeed
    if (!sink.write(data, 10)) return false;
    if (sink.current_size() != 10) return false;
    
    // Writing even 1 more byte should fail
    if (sink.write(data, 1)) return false;
    
    return true;
}
static_assert(test_static_wire_sink_overflow(), "Static WireSink overflow protection failed");

// Test: Static WireSink - clear
constexpr bool test_static_wire_sink_clear() {
    WireSink<256> sink;
    
    const std::uint8_t data[] = {0xAA, 0xBB, 0xCC};
    sink.write(data, 3);
    
    if (sink.current_size() != 3) return false;
    
    sink.clear();
    
    if (sink.current_size() != 0) return false;
    
    // Should be able to write again after clear
    if (!sink.write(data, 3)) return false;
    if (sink.current_size() != 3) return false;
    
    return true;
}
static_assert(test_static_wire_sink_clear(), "Static WireSink clear failed");

// Test: Static WireSink - direct data() access
// Note: Can't be constexpr due to reinterpret_cast limitation in C++20
bool test_static_wire_sink_data_access() {
    WireSink<256> sink;
    
    // Write JSON-like text
    const char json[] = R"({"key":"value"})";
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(json);
    const std::size_t len = std::char_traits<char>::length(json);
    
    if (!sink.write(bytes, len)) return false;
    
    // Access via data()
    const std::uint8_t* data = sink.data();
    if (sink.current_size() != len) return false;
    
    // Verify content
    for (std::size_t i = 0; i < len; ++i) {
        if (data[i] != bytes[i]) return false;
    }
    
    return true;
}
// Runtime test only due to reinterpret_cast
// static_assert(test_static_wire_sink_data_access(), "Static WireSink data access failed");

// Test: Static WireSink - read with offset
constexpr bool test_static_wire_sink_read_offset() {
    WireSink<256> sink;
    
    const std::uint8_t data[] = {0x00, 0x11, 0x22, 0x33, 0x44};
    sink.write(data, 5);
    
    // Read from middle
    std::uint8_t buffer[2] = {};
    if (!sink.read(buffer, 2, 2)) return false;  // Read 2 bytes from offset 2
    
    if (buffer[0] != 0x22) return false;
    if (buffer[1] != 0x33) return false;
    
    // Read beyond current size should fail
    if (sink.read(buffer, 2, 4)) return false;  // offset 4 + count 2 = 6 > size 5
    
    return true;
}
static_assert(test_static_wire_sink_read_offset(), "Static WireSink read with offset failed");

// Test: Dynamic WireSink - basic operations
constexpr bool test_dynamic_wire_sink_basic() {
    WireSink<1024, true> sink;
    
    // Check initial state
    if (sink.current_size() != 0) return false;
    if (sink.max_size() != 1024) return false;
    
    // Write data
    const std::uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    if (!sink.write(data, 4)) return false;
    
    if (sink.current_size() != 4) return false;
    
    // Read back
    std::uint8_t buffer[4] = {};
    if (!sink.read(buffer, 4, 0)) return false;
    
    for (std::size_t i = 0; i < 4; ++i) {
        if (buffer[i] != data[i]) return false;
    }
    
    return true;
}
static_assert(test_dynamic_wire_sink_basic(), "Dynamic WireSink basic operations failed");

// Test: Dynamic WireSink - respects max size
constexpr bool test_dynamic_wire_sink_max_size() {
    WireSink<100, true> sink;  // Max 100 bytes
    
    // Write 50 bytes - should succeed
    std::uint8_t data[50] = {};
    if (!sink.write(data, 50)) return false;
    
    // Write another 50 bytes - should succeed (total = 100)
    if (!sink.write(data, 50)) return false;
    if (sink.current_size() != 100) return false;
    
    // Write even 1 more byte should fail
    if (sink.write(data, 1)) return false;
    
    return true;
}
static_assert(test_dynamic_wire_sink_max_size(), "Dynamic WireSink max size enforcement failed");

// Test: Dynamic WireSink - clear deallocates
constexpr bool test_dynamic_wire_sink_clear() {
    WireSink<1024, true> sink;
    
    const std::uint8_t data[100] = {};
    sink.write(data, 100);
    
    if (sink.current_size() != 100) return false;
    
    sink.clear();
    
    if (sink.current_size() != 0) return false;
    
    // Can write again after clear
    if (!sink.write(data, 100)) return false;
    
    return true;
}
static_assert(test_dynamic_wire_sink_clear(), "Dynamic WireSink clear failed");

// Test: data() method in concept
constexpr bool test_data_method() {
    WireSink<256> sink;
    
    const std::uint8_t data[] = {0xAA, 0xBB, 0xCC};
    sink.write(data, 3);
    
    // Test const data()
    const auto& csink = sink;
    const std::uint8_t* cdata = csink.data();
    if (cdata[0] != 0xAA) return false;
    if (cdata[1] != 0xBB) return false;
    if (cdata[2] != 0xCC) return false;
    
    // Test mutable data()
    std::uint8_t* mdata = sink.data();
    if (mdata[0] != 0xAA) return false;
    
    return true;
}
static_assert(test_data_method(), "data() method failed");

// Test: WireSinkLike concept
static_assert(WireSinkLike<WireSink<256>>, "WireSink<256> should satisfy WireSinkLike");
static_assert(WireSinkLike<WireSink<1024, false>>, "WireSink<1024, false> should satisfy WireSinkLike");
static_assert(WireSinkLike<WireSink<65536, true>>, "WireSink<65536, true> should satisfy WireSinkLike");

// Test: Incremental writes
constexpr bool test_incremental_writes() {
    WireSink<256> sink;
    
    // Write in small chunks
    const std::uint8_t chunk1[] = {0x01, 0x02};
    const std::uint8_t chunk2[] = {0x03, 0x04, 0x05};
    const std::uint8_t chunk3[] = {0x06};
    
    if (!sink.write(chunk1, 2)) return false;
    if (sink.current_size() != 2) return false;
    
    if (!sink.write(chunk2, 3)) return false;
    if (sink.current_size() != 5) return false;
    
    if (!sink.write(chunk3, 1)) return false;
    if (sink.current_size() != 6) return false;
    
    // Verify all data
    std::uint8_t buffer[6] = {};
    if (!sink.read(buffer, 6, 0)) return false;
    
    if (buffer[0] != 0x01) return false;
    if (buffer[1] != 0x02) return false;
    if (buffer[2] != 0x03) return false;
    if (buffer[3] != 0x04) return false;
    if (buffer[4] != 0x05) return false;
    if (buffer[5] != 0x06) return false;
    
    return true;
}
static_assert(test_incremental_writes(), "Incremental writes failed");
