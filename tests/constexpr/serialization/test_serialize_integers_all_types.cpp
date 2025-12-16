#include "test_helpers.hpp"
#include <cstdint>
using namespace TestHelpers;

// ============================================================================
// Signed Integer Types
// ============================================================================

// int8_t
struct Config_int8 { int8_t value; };
static_assert(TestSerialize(Config_int8{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_int8{42}, R"({"value":42})"));
static_assert(TestSerialize(Config_int8{-42}, R"({"value":-42})"));
static_assert(TestSerialize(Config_int8{127}, R"({"value":127})"));    // INT8_MAX
static_assert(TestSerialize(Config_int8{-128}, R"({"value":-128})"));  // INT8_MIN

// int16_t
struct Config_int16 { int16_t value; };
static_assert(TestSerialize(Config_int16{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_int16{1000}, R"({"value":1000})"));
static_assert(TestSerialize(Config_int16{-1000}, R"({"value":-1000})"));
static_assert(TestSerialize(Config_int16{32767}, R"({"value":32767})"));     // INT16_MAX
static_assert(TestSerialize(Config_int16{-32768}, R"({"value":-32768})"));   // INT16_MIN

// int32_t
struct Config_int32 { int32_t value; };
static_assert(TestSerialize(Config_int32{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_int32{100000}, R"({"value":100000})"));
static_assert(TestSerialize(Config_int32{-100000}, R"({"value":-100000})"));
static_assert(TestSerialize(Config_int32{2147483647}, R"({"value":2147483647})"));    // INT32_MAX
static_assert(TestSerialize(Config_int32{-2147483648}, R"({"value":-2147483648})"));  // INT32_MIN

// int64_t
struct Config_int64 { int64_t value; };
static_assert(TestSerialize(Config_int64{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_int64{9000000000LL}, R"({"value":9000000000})"));
static_assert(TestSerialize(Config_int64{-9000000000LL}, R"({"value":-9000000000})"));
static_assert(TestSerialize(Config_int64{9223372036854775807LL}, R"({"value":9223372036854775807})"));    // INT64_MAX
static_assert(TestSerialize(Config_int64{-9223372036854775807LL-1}, R"({"value":-9223372036854775808})")); // INT64_MIN

// ============================================================================
// Unsigned Integer Types
// ============================================================================

// uint8_t
struct Config_uint8 { uint8_t value; };
static_assert(TestSerialize(Config_uint8{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_uint8{42}, R"({"value":42})"));
static_assert(TestSerialize(Config_uint8{255}, R"({"value":255})"));  // UINT8_MAX

// uint16_t
struct Config_uint16 { uint16_t value; };
static_assert(TestSerialize(Config_uint16{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_uint16{1000}, R"({"value":1000})"));
static_assert(TestSerialize(Config_uint16{65535}, R"({"value":65535})"));  // UINT16_MAX

// uint32_t
struct Config_uint32 { uint32_t value; };
static_assert(TestSerialize(Config_uint32{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_uint32{100000}, R"({"value":100000})"));
static_assert(TestSerialize(Config_uint32{4294967295U}, R"({"value":4294967295})"));  // UINT32_MAX

// uint64_t
struct Config_uint64 { uint64_t value; };
static_assert(TestSerialize(Config_uint64{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_uint64{9000000000ULL}, R"({"value":9000000000})"));
static_assert(TestSerialize(Config_uint64{18446744073709551615ULL}, R"({"value":18446744073709551615})"));  // UINT64_MAX

// ============================================================================
// Standard Integer Types
// ============================================================================

// short
struct Config_short { short value; };
static_assert(TestSerialize(Config_short{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_short{100}, R"({"value":100})"));
static_assert(TestSerialize(Config_short{-100}, R"({"value":-100})"));

// int (already tested in test_serialize_int.cpp, but include for completeness)
struct Config_int { int value; };
static_assert(TestSerialize(Config_int{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_int{42}, R"({"value":42})"));
static_assert(TestSerialize(Config_int{-42}, R"({"value":-42})"));

// long
struct Config_long { long value; };
static_assert(TestSerialize(Config_long{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_long{123456}, R"({"value":123456})"));
static_assert(TestSerialize(Config_long{-123456}, R"({"value":-123456})"));

// long long
struct Config_longlong { long long value; };
static_assert(TestSerialize(Config_longlong{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_longlong{9876543210LL}, R"({"value":9876543210})"));
static_assert(TestSerialize(Config_longlong{-9876543210LL}, R"({"value":-9876543210})"));

// unsigned short
struct Config_ushort { unsigned short value; };
static_assert(TestSerialize(Config_ushort{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_ushort{1000}, R"({"value":1000})"));

// unsigned int
struct Config_uint { unsigned int value; };
static_assert(TestSerialize(Config_uint{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_uint{424242}, R"({"value":424242})"));

// unsigned long
struct Config_ulong { unsigned long value; };
static_assert(TestSerialize(Config_ulong{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_ulong{9999999}, R"({"value":9999999})"));

// unsigned long long
struct Config_ulonglong { unsigned long long value; };
static_assert(TestSerialize(Config_ulonglong{0}, R"({"value":0})"));
static_assert(TestSerialize(Config_ulonglong{12345678901234ULL}, R"({"value":12345678901234})"));

// ============================================================================
// Roundtrip Tests
// ============================================================================

// Verify serialize → parse roundtrip for key types
static_assert(TestRoundTrip<Config_int8>(R"({"value":-128})", Config_int8{-128}));
static_assert(TestRoundTrip<Config_uint8>(R"({"value":255})", Config_uint8{255}));
static_assert(TestRoundTrip<Config_int32>(R"({"value":-2147483648})", Config_int32{-2147483648}));
static_assert(TestRoundTrip<Config_uint32>(R"({"value":4294967295})", Config_uint32{4294967295U}));
static_assert(TestRoundTrip<Config_int64>(R"({"value":9223372036854775807})", Config_int64{9223372036854775807LL}));
static_assert(TestRoundTrip<Config_uint64>(R"({"value":18446744073709551615})", Config_uint64{18446744073709551615ULL}));

// ============================================================================
// Multi-field Tests
// ============================================================================

struct AllIntegers {
    int8_t i8;
    uint8_t u8;
    int16_t i16;
    uint16_t u16;
    int32_t i32;
    uint32_t u32;
};

static_assert(TestSerialize(
    AllIntegers{-1, 255, -1000, 60000, -100000, 4000000000U},
    R"({"i8":-1,"u8":255,"i16":-1000,"u16":60000,"i32":-100000,"u32":4000000000})"
));

static_assert(TestRoundTrip<AllIntegers>(
    R"({"i8":127,"u8":255,"i16":32767,"u16":65535,"i32":2147483647,"u32":4294967295})",
    AllIntegers{127, 255, 32767, 65535, 2147483647, 4294967295U}
));

