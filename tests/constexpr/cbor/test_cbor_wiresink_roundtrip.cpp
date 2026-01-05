// Compile-time test: CBOR WireSink integration with roundtrip and transformer
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/cbor.hpp>
#include <array>

using namespace JsonFusion;
using namespace JsonFusion::options;

// Test 0a: Simple integer to isolate byte-order issue
constexpr bool test_cbor_simple_integer() {
    // Test uint16_t value: 8080 = 0x1F90
    // Expected CBOR: 19 1F 90 (ai=25, then 2-byte big-endian)
    std::uint16_t value1 = 8080;
    
    std::array<char, 64> buffer{};
    auto it = buffer.begin();
    
    auto res = SerializeWithWriter(value1, CborWriter (it, buffer.end()));
    if (!res) return false;
    
    std::size_t written = res.bytesWritten();
    
    std::uint16_t value2 = 0;
    auto begin = buffer.begin();
    if (!ParseWithReader(value2, CborReader (begin, buffer.begin() + written))) return false;
    if (value1 != value2) return false;
    
    return true;
}

// Test 0b: Sign extension issue (values >= 128)
// This test ensures char* iterators (signed) don't cause sign extension bugs
constexpr bool test_cbor_sign_extension() {
    // Test case 1: uint8_t value 255 (0xFF) → CBOR: 18 FF (ai=24)
    // Without proper uint8_t cast, signed char -1 would sign-extend to 0xFFFFFFFFFFFFFFFF
    std::uint64_t value1 = 255;
    std::array<char, 64> buffer1{};
    auto it1 = buffer1.begin();
    auto res1 = SerializeWithWriter(value1, CborWriter(it1, buffer1.end()));
    if (!res1) return false;
    
    std::uint64_t value2 = 0;
    auto begin1 = buffer1.begin();
    if (!ParseWithReader(value2, CborReader(begin1, begin1 + res1.bytesWritten()))) return false;
    if (value1 != value2) return false;
    
    // Test case 2: uint16_t with high byte >= 128 → CBOR: 19 FF 90 (ai=25)
    // Without proper uint8_t cast, 0xFF would sign-extend during shift operations
    std::uint16_t value3 = 0xFF90;  // 65424
    std::array<char, 64> buffer2{};
    auto it2 = buffer2.begin();
    auto res2 = SerializeWithWriter(value3, CborWriter (it2, buffer2.end()));
    if (!res2) return false;
    
    std::uint16_t value4 = 0;
    auto begin2 = buffer2.begin();
    if (!ParseWithReader(value4, CborReader(begin2, begin2 + res2.bytesWritten()))) return false;
    if (value3 != value4) return false;
    
    return true;
}

// Simple test model
struct NetworkConfig {
    std::array<char, 16> name{};
    std::uint16_t port = 0;
    bool enabled = false;
};

// Helper to create string from literal
constexpr std::array<char, 16> make_name(const char* str) {
    std::array<char, 16> result{};
    for (std::size_t i = 0; i < 16 && str[i] != '\0'; ++i) {
        result[i] = str[i];
    }
    return result;
}

// Helper to compare arrays
constexpr bool compare_arrays(const std::array<char, 16>& a, const std::array<char, 16>& b) {
    for (std::size_t i = 0; i < 16; ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// Test 1: Basic roundtrip  
constexpr bool test_cbor_basic_roundtrip() {
    // Create config
    NetworkConfig config1;
    config1.name = make_name("test-network");
    config1.port = 8080;
    config1.enabled = true;
    
    // Serialize to CBOR
    std::array<char, 256> buffer{};
    auto it = buffer.begin();
    auto result = SerializeWithWriter(config1, CborWriter(it, buffer.end()));
    if (!result) {
        return false;
    }
    
    std::size_t written = result.bytesWritten();
    
    // Parse back with CBOR
    NetworkConfig config2;
    auto begin = buffer.begin();
    auto parse_result = ParseWithReader(config2, CborReader(begin, buffer.begin() + written));
    if (!parse_result) {
        return false;
    }
    
    
    if (!compare_arrays(config1.name, config2.name)) {
        return false;
    }
    if (config1.port != config2.port) {
        return false;
    }
    if (config1.enabled != config2.enabled) {
        return false;
    }
    
    return true;
}

// Test 2: WireSink capture and output
constexpr bool test_cbor_wiresink_capture() {
    // Serialize config to CBOR
    NetworkConfig config;
    config.name = make_name("wifi-ap");
    config.port = 443;
    config.enabled = false;
    
    std::array<char, 256> buffer{};
    auto it = buffer.begin();
    auto ser_result = SerializeWithWriter(config, CborWriter(it, buffer.end()));
    if (!ser_result) {
        return false;
    }
    
    std::size_t cbor_size = ser_result.bytesWritten();
    
    // Parse and capture to WireSink
    WireSink<128> sink;
    {
        auto begin = buffer.begin();
        CborReader reader(begin, buffer.begin() + cbor_size);
        
        if (!reader.capture_to_sink(sink)) {
            return false;
        }
    }
    
    
    // Verify sink captured the right amount
    if (sink.current_size() != cbor_size) {
        return false;
    }
    
    // Output from sink to new buffer
    std::array<char, 256> buffer2{};
    {
        auto begin = buffer2.begin();
        CborWriter writer(begin, buffer2.end());
        
        if (!writer.output_from_sink(sink)) {
            return false;
        }
    }
    
    
    // Parse from new buffer with CBOR and verify
    NetworkConfig config2{};  // Zero-initialize to avoid garbage in unused bytes
    auto begin2 = buffer2.begin();
    auto parse_result = ParseWithReader(config2, CborReader(begin2, buffer2.begin() + cbor_size));
    if (!parse_result) {
        return false;
    }
    
    if (!compare_arrays(config.name, config2.name)) return false;
    if (config.port != config2.port) return false;
    if (config.enabled != config2.enabled) return false;
    
    return true;
}

// Test 3: WireSink with transformer (schema evolution)
struct LegacyConfig {
    std::array<char, 16> name;
    std::uint16_t port;
    // Old: no "enabled" field
};

struct ModernConfig {
    std::array<char, 16> name;
    std::uint16_t port;
    
    // New field with backward compatibility transformer
    struct EnabledField {
        using wire_type = WireSink<64>;
        bool value = true;  // Default for legacy configs
        
        constexpr bool transform_from(const auto& parseFn) {
            // Try parsing as bool (new format)
            if (parseFn(value)) {
                return true;
            }
            // Old format: field missing, use default
            value = true;
            return true;
        }
        
        constexpr bool transform_to(const auto& serializeFn) const {
            // Always serialize as bool (new format)
            return serializeFn(value);
        }
    };
    
    EnabledField enabled;
};

constexpr bool test_cbor_wiresink_transformer() {
    // Serialize legacy config (no "enabled" field) with CBOR
    LegacyConfig legacy;
    legacy.name = make_name("old-system");
    legacy.port = 9000;
    
    std::array<char, 256> legacy_buffer{};
    auto it = legacy_buffer.begin();
    auto res = SerializeWithWriter(legacy, CborWriter(it, legacy_buffer.end()));
    if (!res) return false;
    
    std::size_t legacy_size = res.bytesWritten();
    
    // Parse as modern config with CBOR (transformer handles missing field)
    ModernConfig modern{};  // Zero-initialize
    auto begin = legacy_buffer.begin();
    auto result = ParseWithReader(modern, CborReader(begin, legacy_buffer.begin() + legacy_size));
    
    // Should succeed with default value
    if (!result) return false;
    if (!compare_arrays(legacy.name, modern.name)) return false;
    if (legacy.port != modern.port) return false;
    if (modern.enabled.value != true) return false;  // Default value
    
    // Now serialize modern config (includes "enabled") with CBOR
    modern.enabled.value = false;
    
    std::array<char, 256> modern_buffer{};
    auto it2 = modern_buffer.begin();
    auto mres = SerializeWithWriter(modern, CborWriter(it2, modern_buffer.end()));
    if (!mres) return false;
    
    std::size_t modern_size = mres.bytesWritten();
    
    // Parse back with CBOR and verify
    ModernConfig modern2{};  // Zero-initialize
    auto begin2 = modern_buffer.begin();
    if (!ParseWithReader(modern2, CborReader(begin2, modern_buffer.begin() + modern_size))) return false;
    
    if (!compare_arrays(modern.name, modern2.name)) return false;
    if (modern.port != modern2.port) return false;
    if (modern.enabled.value != modern2.enabled.value) return false;
    
    return true;
}
static_assert(test_cbor_simple_integer(), "Simple integer roundtrip");
static_assert(test_cbor_sign_extension(), "Sign extension");
static_assert(test_cbor_basic_roundtrip(), "Basic roundtrip");
static_assert(test_cbor_wiresink_capture(), "WireSink capture");
static_assert(test_cbor_wiresink_transformer(), "WireSink transformer");
