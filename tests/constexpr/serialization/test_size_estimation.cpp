#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/serialize_size_estimator.hpp>
#include <array>
#include <optional>

using namespace JsonFusion;
using namespace JsonFusion::size_estimator;

// ============================================================================
// Test: Bool size estimation
// ============================================================================

static_assert(EstimateMaxSerializedSize<bool>() == 5,
              "Bool max size should be 5 (\"false\")");

// ============================================================================
// Test: Integer size estimation
// ============================================================================

// int8_t: -128 to 127 -> max 4 chars ("-128")
static_assert(EstimateMaxSerializedSize<int8_t>() >= 4,
              "int8_t should need at least 4 bytes");

// int16_t: -32768 to 32767 -> max 6 chars ("-32768")
static_assert(EstimateMaxSerializedSize<int16_t>() >= 6,
              "int16_t should need at least 6 bytes");

// int32_t: max ~11 chars for "-2147483648"
static_assert(EstimateMaxSerializedSize<int32_t>() >= 11,
              "int32_t should need at least 11 bytes");

// uint8_t: 0 to 255 -> max 3 chars
static_assert(EstimateMaxSerializedSize<uint8_t>() >= 3,
              "uint8_t should need at least 3 bytes");

// ============================================================================
// Test: Float size estimation
// ============================================================================

static_assert(EstimateMaxSerializedSize<float>() >= 10,
              "float should have reasonable size estimate");

static_assert(EstimateMaxSerializedSize<double>() >= 15,
              "double should have reasonable size estimate");

// ============================================================================
// Test: Fixed string size estimation
// ============================================================================

// String of N chars: 2 (quotes) + 2*N (worst case escaping)
struct StringStruct {
    std::array<char, 10> name{};
};

constexpr std::size_t string_size = EstimateMaxSerializedSize<StringStruct>();
// {"name":"..."} = 1 + 6 + 2 + 22 (2*10+2 for escaped string) + 1 = ~32 minimum
static_assert(string_size >= 30,
              "String struct should have adequate size estimate");

// ============================================================================
// Test: Fixed array size estimation
// ============================================================================

struct ArrayStruct {
    std::array<int, 5> values{};
};

constexpr std::size_t array_size = EstimateMaxSerializedSize<ArrayStruct>();
// {"values":[n,n,n,n,n]} where each n is up to 11 chars
// Minimum: 1 + 8 + 2 + 1 + 5*1 + 4 + 1 + 1 = ~23 for small ints
static_assert(array_size >= 20,
              "Array struct should have adequate size estimate");

// ============================================================================
// Test: Nested struct size estimation
// ============================================================================

struct Inner {
    int x;
    int y;
};

struct Outer {
    Inner point;
    bool active;
};

constexpr std::size_t nested_size = EstimateMaxSerializedSize<Outer>();
// Should be larger than sum of parts due to JSON overhead
static_assert(nested_size >= 30,
              "Nested struct should have adequate size estimate");

// ============================================================================
// Test: Optional adds size for value (not "null" overhead)
// ============================================================================

struct OptionalStruct {
    std::optional<int> maybe;
};

constexpr std::size_t optional_size = EstimateMaxSerializedSize<OptionalStruct>();
// {"maybe":...} - must account for worst case (int value)
static_assert(optional_size >= 15,
              "Optional struct should account for value size");

// ============================================================================
// Test: Size estimate is conservative (actual <= estimate)
// ============================================================================

constexpr bool test_estimate_is_conservative() {
    struct Config {
        std::array<char, 16> name{};
        int port;
        bool enabled;
    };

    Config c;
    // Fill with some data
    c.name[0] = 't'; c.name[1] = 'e'; c.name[2] = 's'; c.name[3] = 't'; c.name[4] = '\0';
    c.port = 8080;
    c.enabled = true;

    constexpr std::size_t estimated = EstimateMaxSerializedSize<Config>();

    // Create buffer with estimated size
    std::array<char, estimated> buffer{};

    // Serialize should succeed (buffer is large enough)
    auto result = Serialize(c, buffer.data(), buffer.data() + buffer.size());
    if (!result) return false;

    // Actual size should be <= estimate
    std::size_t actual = result.pos() - buffer.data();
    if (actual > estimated) return false;

    return true;
}
static_assert(test_estimate_is_conservative(),
              "Size estimate should be conservative (actual <= estimate)");

// ============================================================================
// Test: Empty struct
// ============================================================================

struct EmptyStruct {};

constexpr std::size_t empty_size = EstimateMaxSerializedSize<EmptyStruct>();
static_assert(empty_size >= 2,
              "Empty struct should be at least {} = 2 bytes");

// ============================================================================
// Test: Multiple string fields
// ============================================================================

struct MultiString {
    std::array<char, 8> first{};
    std::array<char, 8> second{};
};

constexpr std::size_t multi_string_size = EstimateMaxSerializedSize<MultiString>();
static_assert(multi_string_size >= 40,
              "Multiple string fields should accumulate size");

// ============================================================================
// Test: 2D array estimation
// ============================================================================

struct Matrix {
    std::array<std::array<int, 3>, 3> data{};
};

constexpr std::size_t matrix_size = EstimateMaxSerializedSize<Matrix>();
// 9 ints with nesting overhead
static_assert(matrix_size >= 50,
              "2D array should have adequate size estimate");

// ============================================================================
// Test: Actual serialization fits in estimated buffer
// ============================================================================

constexpr bool test_serialize_fits_in_buffer() {
    struct TestData {
        std::array<char, 20> msg{};
        std::array<int, 5> nums{};
        bool flag;
    };

    TestData data;
    data.msg[0] = 'H'; data.msg[1] = 'i'; data.msg[2] = '\0';
    data.nums = {1, 2, 3, 4, 5};
    data.flag = true;

    constexpr std::size_t buffer_size = EstimateMaxSerializedSize<TestData>();
    std::array<char, buffer_size> buffer{};

    auto result = Serialize(data, buffer.data(), buffer.data() + buffer.size());
    return static_cast<bool>(result);
}
static_assert(test_serialize_fits_in_buffer(),
              "Serialization should fit in estimated buffer");

