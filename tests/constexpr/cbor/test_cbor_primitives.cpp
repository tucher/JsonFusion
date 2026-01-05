#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/cbor.hpp>
#include <array>
#include <optional>
using namespace JsonFusion;

// Helper for roundtrip test: serialize value, deserialize it back, compare with operator==
template<typename T>
constexpr bool test_cbor_value_roundtrip(T value) {
    std::array<char, 256> buffer{};
    auto it = buffer.begin();
    auto res = SerializeWithWriter(value, CborWriter(it, buffer.end()));
    if (!res) return false;
    T value2{};
    if (!ParseWithReader(value2, CborReader(buffer.begin(), buffer.begin() + res.bytesWritten()))) return false;
    return value == value2;
}

// ========== Test structures (must be at namespace scope) ==========

struct SimpleStruct {
    int x;
    bool y;
    
    constexpr bool operator==(const SimpleStruct& other) const {
        return x == other.x && y == other.y;
    }
};

struct MixedStruct {
    int a;
    double b;
    bool c;
    std::string d;
    
    constexpr bool operator==(const MixedStruct& other) const {
        return a == other.a && b == other.b && c == other.c && d == other.d;
    }
};

struct StructWithArray {
    std::vector<int> numbers;
    std::string name;
    
    constexpr bool operator==(const StructWithArray& other) const {
        return numbers == other.numbers && name == other.name;
    }
};

struct WithOptionals {
    int required;
    std::optional<int> opt_int;
    std::optional<std::string> opt_str;
    
    constexpr bool operator==(const WithOptionals& other) const {
        return required == other.required && opt_int == other.opt_int && opt_str == other.opt_str;
    }
};

// ========== Integer tests - various sizes and edge cases ==========

// Small positive integers
static_assert(test_cbor_value_roundtrip<std::uint8_t>(0));
static_assert(test_cbor_value_roundtrip<std::uint8_t>(1));
static_assert(test_cbor_value_roundtrip<std::uint8_t>(23));
static_assert(test_cbor_value_roundtrip<std::uint8_t>(24));
static_assert(test_cbor_value_roundtrip<std::uint8_t>(255));

static_assert(test_cbor_value_roundtrip<std::uint16_t>(0));
static_assert(test_cbor_value_roundtrip<std::uint16_t>(256));
static_assert(test_cbor_value_roundtrip<std::uint16_t>(65535));

static_assert(test_cbor_value_roundtrip<std::uint32_t>(0));
static_assert(test_cbor_value_roundtrip<std::uint32_t>(65536));
static_assert(test_cbor_value_roundtrip<std::uint32_t>(0xFFFFFFFF));

static_assert(test_cbor_value_roundtrip<std::uint64_t>(0));
static_assert(test_cbor_value_roundtrip<std::uint64_t>(0x100000000ULL));
static_assert(test_cbor_value_roundtrip<std::uint64_t>(0xFFFFFFFFFFFFFFFFULL));

// Signed integers - positive, negative, and edge cases
static_assert(test_cbor_value_roundtrip<std::int8_t>(0));
static_assert(test_cbor_value_roundtrip<std::int8_t>(1));
static_assert(test_cbor_value_roundtrip<std::int8_t>(-1));
static_assert(test_cbor_value_roundtrip<std::int8_t>(127));
static_assert(test_cbor_value_roundtrip<std::int8_t>(-128));

static_assert(test_cbor_value_roundtrip<std::int16_t>(0));
static_assert(test_cbor_value_roundtrip<std::int16_t>(32767));
static_assert(test_cbor_value_roundtrip<std::int16_t>(-32768));

static_assert(test_cbor_value_roundtrip<std::int32_t>(0));
static_assert(test_cbor_value_roundtrip<std::int32_t>(2147483647));
static_assert(test_cbor_value_roundtrip<std::int32_t>(-2147483648));

static_assert(test_cbor_value_roundtrip<std::int64_t>(0));
static_assert(test_cbor_value_roundtrip<std::int64_t>(9223372036854775807LL));
static_assert(test_cbor_value_roundtrip<std::int64_t>(-9223372036854775807LL - 1));

// ========== Floating point tests ==========

static_assert(test_cbor_value_roundtrip<float>(0.0f));
static_assert(test_cbor_value_roundtrip<float>(1.0f));
static_assert(test_cbor_value_roundtrip<float>(-1.0f));
static_assert(test_cbor_value_roundtrip<float>(3.14159f));
static_assert(test_cbor_value_roundtrip<float>(-3.14159f));

static_assert(test_cbor_value_roundtrip<double>(0.0));
static_assert(test_cbor_value_roundtrip<double>(1.0));
static_assert(test_cbor_value_roundtrip<double>(-1.0));
static_assert(test_cbor_value_roundtrip<double>(3.141592653589793));
static_assert(test_cbor_value_roundtrip<double>(-2.718281828459045));

// ========== Boolean tests ==========

static_assert(test_cbor_value_roundtrip<bool>(true));
static_assert(test_cbor_value_roundtrip<bool>(false));

// ========== String tests ==========

static_assert(test_cbor_value_roundtrip<std::string>(""));
static_assert(test_cbor_value_roundtrip<std::string>("hello"));
static_assert(test_cbor_value_roundtrip<std::string>("Hello, CBOR World!"));
static_assert(test_cbor_value_roundtrip<std::string>("a"));
static_assert(test_cbor_value_roundtrip<std::string>("1234567890abcdefghijklmnopqrstuvwxyz"));

// ========== Array tests ==========

// std::vector
static_assert(test_cbor_value_roundtrip<std::vector<int>>({}));
static_assert(test_cbor_value_roundtrip<std::vector<int>>({1}));
static_assert(test_cbor_value_roundtrip<std::vector<int>>({1, 2, 3}));
static_assert(test_cbor_value_roundtrip<std::vector<int>>({-1, 0, 1, 2, 3}));
// Note: nested vector initializers don't work well in constexpr context
static_assert([](){
    return test_cbor_value_roundtrip<std::vector<std::string>>({{"hello"}, {"world"}});
}());

// std::array
static_assert(test_cbor_value_roundtrip<std::array<int, 0>>({}));
static_assert(test_cbor_value_roundtrip<std::array<int, 1>>({42}));
static_assert(test_cbor_value_roundtrip<std::array<int, 3>>({1, 2, 3}));
static_assert(test_cbor_value_roundtrip<std::array<int, 5>>({10, 20, 30, 40, 50}));

// Nested arrays
// Note: nested vector initializers don't work well in constexpr context with g++-14
static_assert([](){
    return test_cbor_value_roundtrip<std::vector<std::vector<int>>>({{1, 2}, {3, 4, 5}});
}());
static_assert(test_cbor_value_roundtrip<std::array<std::array<int, 2>, 2>>({{{1, 2}, {3, 4}}}));

// ========== Optional tests ==========

static_assert(test_cbor_value_roundtrip<std::optional<int>>(std::nullopt));
static_assert(test_cbor_value_roundtrip<std::optional<int>>(42));
static_assert(test_cbor_value_roundtrip<std::optional<std::string>>(std::nullopt));
static_assert(test_cbor_value_roundtrip<std::optional<std::string>>("test"));
static_assert(test_cbor_value_roundtrip<std::optional<bool>>(true));
static_assert(test_cbor_value_roundtrip<std::optional<bool>>(false));

// ========== Struct (map) tests ==========

// Simple struct
static_assert(test_cbor_value_roundtrip(SimpleStruct{1, true}));
static_assert(test_cbor_value_roundtrip(SimpleStruct{0, false}));
static_assert(test_cbor_value_roundtrip(SimpleStruct{-42, true}));

// Struct with various types
static_assert(test_cbor_value_roundtrip(MixedStruct{42, 3.14, true, "test"}));
static_assert(test_cbor_value_roundtrip(MixedStruct{0, 0.0, false, ""}));

// Struct with arrays
static_assert(test_cbor_value_roundtrip(StructWithArray{{1, 2, 3}, "test"}));
static_assert(test_cbor_value_roundtrip(StructWithArray{{}, "empty"}));

// Struct with optional fields
static_assert(test_cbor_value_roundtrip(WithOptionals{1, 2, "test"}));
static_assert(test_cbor_value_roundtrip(WithOptionals{1, std::nullopt, std::nullopt}));
static_assert(test_cbor_value_roundtrip(WithOptionals{42, 100, std::nullopt}));
static_assert(test_cbor_value_roundtrip(WithOptionals{99, std::nullopt, "only_string"}));
