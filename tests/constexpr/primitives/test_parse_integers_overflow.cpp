#include "test_helpers.hpp"
using namespace TestHelpers;
#include <cstdint>

// ===== int8_t overflow =====
struct ConfigInt8 { int8_t value; };

// Overflow above INT8_MAX (127)
static_assert(TestParseError<ConfigInt8>(R"({"value": 128})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigInt8>(R"({"value": 200})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigInt8>(R"({"value": 32767})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// Underflow below INT8_MIN (-128)
static_assert(TestParseError<ConfigInt8>(R"({"value": -129})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigInt8>(R"({"value": -200})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigInt8>(R"({"value": -32768})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// ===== uint8_t overflow =====
struct ConfigUInt8 { uint8_t value; };

// Overflow above UINT8_MAX (255)
static_assert(TestParseError<ConfigUInt8>(R"({"value": 256})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigUInt8>(R"({"value": 300})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigUInt8>(R"({"value": 65535})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// Negative values for unsigned types
static_assert(TestParseError<ConfigUInt8>(R"({"value": -1})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigUInt8>(R"({"value": -128})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// ===== int16_t overflow =====
struct ConfigInt16 { int16_t value; };

// Overflow above INT16_MAX (32767)
static_assert(TestParseError<ConfigInt16>(R"({"value": 32768})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigInt16>(R"({"value": 40000})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigInt16>(R"({"value": 2147483647})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// Underflow below INT16_MIN (-32768)
static_assert(TestParseError<ConfigInt16>(R"({"value": -32769})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigInt16>(R"({"value": -40000})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// ===== uint16_t overflow =====
struct ConfigUInt16 { uint16_t value; };

// Overflow above UINT16_MAX (65535)
static_assert(TestParseError<ConfigUInt16>(R"({"value": 65536})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigUInt16>(R"({"value": 70000})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigUInt16>(R"({"value": 4294967295})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// Negative values for unsigned types
static_assert(TestParseError<ConfigUInt16>(R"({"value": -1})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigUInt16>(R"({"value": -32768})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// ===== int32_t overflow =====
struct ConfigInt32 { int32_t value; };

// Overflow above INT32_MAX (2147483647)
static_assert(TestParseError<ConfigInt32>(R"({"value": 2147483648})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigInt32>(R"({"value": 3000000000})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigInt32>(R"({"value": 9223372036854775807})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// Underflow below INT32_MIN (-2147483648)
static_assert(TestParseError<ConfigInt32>(R"({"value": -2147483649})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigInt32>(R"({"value": -3000000000})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// ===== uint32_t overflow =====
struct ConfigUInt32 { uint32_t value; };

// Overflow above UINT32_MAX (4294967295)
static_assert(TestParseError<ConfigUInt32>(R"({"value": 4294967296})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigUInt32>(R"({"value": 5000000000})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigUInt32>(R"({"value": 18446744073709551615})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// Negative values for unsigned types
static_assert(TestParseError<ConfigUInt32>(R"({"value": -1})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigUInt32>(R"({"value": -2147483648})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// ===== int64_t overflow =====
struct ConfigInt64 { int64_t value; };

// Overflow above INT64_MAX (9223372036854775807)
static_assert(TestParseError<ConfigInt64>(R"({"value": 9223372036854775808})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigInt64>(R"({"value": 99999999999999999999})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// Underflow below INT64_MIN (-9223372036854775808)
static_assert(TestParseError<ConfigInt64>(R"({"value": -9223372036854775809})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigInt64>(R"({"value": -99999999999999999999})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// ===== uint64_t overflow =====
struct ConfigUInt64 { uint64_t value; };

// Overflow above UINT64_MAX (18446744073709551615)
static_assert(TestParseError<ConfigUInt64>(R"({"value": 18446744073709551616})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigUInt64>(R"({"value": 99999999999999999999})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// Negative values for unsigned types
static_assert(TestParseError<ConfigUInt64>(R"({"value": -1})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<ConfigUInt64>(R"({"value": -9223372036854775808})", JsonFusion::ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

