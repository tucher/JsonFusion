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
    const char data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    if (!sink.write(data, 5)) return false;
    
    if (sink.current_size() != 5) return false;
    
    // Read it back
    char buffer[5] = {};
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
    
    const char data[20] = {};
    
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
    
    const char data[] = {char(0xAA), char(0xBB), char(0xCC)};
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
constexpr bool test_static_wire_sink_data_access() {
    WireSink<256> sink;
    
    // Write JSON-like text
    const char json[] = R"({"key":"value"})";
    
    if (!sink.write(json, sizeof(json))) return false;
    
    // Access via data()
    const char* data = sink.data();
    if (sink.current_size() != sizeof(json)) return false;
    
    // Verify content
    for (std::size_t i = 0; i < sizeof(json); ++i) {
        if (data[i] != json[i]) return false;
    }
    
    return true;
}
// Runtime test only due to reinterpret_cast
static_assert(test_static_wire_sink_data_access(), "Static WireSink data access failed");

// Test: Static WireSink - read with offset
constexpr bool test_static_wire_sink_read_offset() {
    WireSink<256> sink;
    
    const char data[] = {0x00, 0x11, 0x22, 0x33, 0x44};
    sink.write(data, 5);
    
    // Read from middle
    char buffer[2] = {};
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
    const char data[] = {char(0xDE), char(0xAD), char(0xBE), char(0xEF)};
    if (!sink.write(data, 4)) return false;
    
    if (sink.current_size() != 4) return false;
    
    // Read back
    char buffer[4] = {};
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
    char data[50] = {};
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
    
    const char data[100] = {};
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
    
    const char data[] = {char(0xAA), char(0xBB), char(0xCC)};
    sink.write(data, 3);
    
    // Test const data()
    const auto& csink = sink;
    const char* cdata = csink.data();
    if (cdata[0] != char(0xAA)) return false;
    if (cdata[1] != char(0xBB)) return false;
    if (cdata[2] != char(0xCC)) return false;
    
    // Test mutable data()
    char* mdata = sink.data();
    if (mdata[0] != char(0xAA)) return false;
    
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
    const char chunk1[] = {0x01, 0x02};
    const char chunk2[] = {0x03, 0x04, 0x05};
    const char chunk3[] = {0x06};
    
    if (!sink.write(chunk1, 2)) return false;
    if (sink.current_size() != 2) return false;
    
    if (!sink.write(chunk2, 3)) return false;
    if (sink.current_size() != 5) return false;
    
    if (!sink.write(chunk3, 1)) return false;
    if (sink.current_size() != 6) return false;
    
    // Verify all data
    char buffer[6] = {};
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
