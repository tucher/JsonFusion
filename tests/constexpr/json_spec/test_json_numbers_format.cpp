#include "test_helpers.hpp"
using namespace TestHelpers;

// ============================================================================
// Valid Integer Formats (RFC 8259 Compliance)
// ============================================================================

struct Config_int { int value; };

// Zero
static_assert(TestParse(R"({"value":0})", Config_int{0}));

// Positive integers
static_assert(TestParse(R"({"value":42})", Config_int{42}));
static_assert(TestParse(R"({"value":123})", Config_int{123}));
static_assert(TestParse(R"({"value":9999})", Config_int{9999}));

// Negative integers
static_assert(TestParse(R"({"value":-1})", Config_int{-1}));
static_assert(TestParse(R"({"value":-123})", Config_int{-123}));
static_assert(TestParse(R"({"value":-9999})", Config_int{-9999}));

// Negative zero (valid, treated as 0)
static_assert(TestParse(R"({"value":-0})", Config_int{0}));

// Large integers
static_assert(TestParse(R"({"value":2147483647})", Config_int{2147483647}));   // INT32_MAX
static_assert(TestParse(R"({"value":-2147483648})", Config_int{-2147483648})); // INT32_MIN

// ============================================================================
// Invalid Integer Formats (Must Reject)
// ============================================================================

// Leading zeros (invalid per RFC 8259)
static_assert(TestParseError<Config_int>(R"({"value":00})", JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER));
static_assert(TestParseError<Config_int>(R"({"value":01})", JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER));
static_assert(TestParseError<Config_int>(R"({"value":0123})", JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER));
static_assert(TestParseError<Config_int>(R"({"value":007})", JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER));

// Positive sign (invalid in JSON)
static_assert(TestParseError<Config_int>(R"({"value":+42})", JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER));
static_assert(TestParseError<Config_int>(R"({"value":+0})", JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER));

// Invalid number formats (non-JSON)
static_assert(TestParseError<Config_int>(R"({"value":0x123})", JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER));  // Hex
static_assert(TestParseError<Config_int>(R"({"value":0b101})", JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER));  // Binary
static_assert(TestParseError<Config_int>(R"({"value":1_000})", JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER));  // Underscore separator

// ============================================================================
// Integer Type Ranges
// ============================================================================

// int8_t
struct Config_int8 { int8_t value; };
static_assert(TestParse(R"({"value":127})", Config_int8{127}));    // INT8_MAX
static_assert(TestParse(R"({"value":-128})", Config_int8{-128}));  // INT8_MIN
static_assert(TestParse(R"({"value":0})", Config_int8{0}));

// int16_t
struct Config_int16 { int16_t value; };
static_assert(TestParse(R"({"value":32767})", Config_int16{32767}));     // INT16_MAX
static_assert(TestParse(R"({"value":-32768})", Config_int16{-32768}));   // INT16_MIN
static_assert(TestParse(R"({"value":0})", Config_int16{0}));

// int64_t
struct Config_int64 { int64_t value; };
static_assert(TestParse(R"({"value":9223372036854775807})", Config_int64{9223372036854775807LL}));    // INT64_MAX
static_assert(TestParse(R"({"value":-9223372036854775808})", Config_int64{-9223372036854775807LL-1})); // INT64_MIN
static_assert(TestParse(R"({"value":0})", Config_int64{0}));

// uint8_t
struct Config_uint8 { uint8_t value; };
static_assert(TestParse(R"({"value":255})", Config_uint8{255}));  // UINT8_MAX
static_assert(TestParse(R"({"value":0})", Config_uint8{0}));

// uint16_t
struct Config_uint16 { uint16_t value; };
static_assert(TestParse(R"({"value":65535})", Config_uint16{65535}));  // UINT16_MAX
static_assert(TestParse(R"({"value":0})", Config_uint16{0}));

// uint32_t
struct Config_uint32 { uint32_t value; };
static_assert(TestParse(R"({"value":4294967295})", Config_uint32{4294967295U}));  // UINT32_MAX
static_assert(TestParse(R"({"value":0})", Config_uint32{0}));

// uint64_t
struct Config_uint64 { uint64_t value; };
static_assert(TestParse(R"({"value":18446744073709551615})", Config_uint64{18446744073709551615ULL}));  // UINT64_MAX
static_assert(TestParse(R"({"value":0})", Config_uint64{0}));

// ============================================================================
// Overflow Detection
// ============================================================================

// Overflow for int32 (value too large)
static_assert(TestParseError<Config_int>(R"({"value":9999999999999999})", JsonFusion::JsonIteratorReaderError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// Underflow for int32 (value too small)
static_assert(TestParseError<Config_int>(R"({"value":-9999999999999999})", JsonFusion::JsonIteratorReaderError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// Overflow for uint8 (256 > UINT8_MAX)
static_assert(TestParseError<Config_uint8>(R"({"value":256})", JsonFusion::JsonIteratorReaderError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// Negative for unsigned type
static_assert(TestParseError<Config_uint8>(R"({"value":-1})", JsonFusion::JsonIteratorReaderError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));
static_assert(TestParseError<Config_uint32>(R"({"value":-1})", JsonFusion::JsonIteratorReaderError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE));

// ============================================================================
// Floating-Point Formats (Basic - Full tests in dedicated FP test file)
// ============================================================================

struct Config_double { double value; };

// Valid decimals
static_assert(TestParse(R"({"value":0.0})", Config_double{0.0}));
static_assert(TestParse(R"({"value":3.14})", Config_double{3.14}));
static_assert(TestParse(R"({"value":-2.5})", Config_double{-2.5}));
static_assert(TestParse(R"({"value":123.456})", Config_double{123.456}));

// Leading zero in decimal (valid: 0.5)
static_assert(TestParse(R"({"value":0.5})", Config_double{0.5}));

// Scientific notation
static_assert(TestParse(R"({"value":1e10})", Config_double{1e10}));
static_assert(TestParse(R"({"value":1.5e-5})", Config_double{1.5e-5}));
static_assert(TestParse(R"({"value":2E+3})", Config_double{2E+3}));

// Invalid floating-point formats
static_assert(TestParseError<Config_double>(R"({"value":42.})", JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER));   // Trailing dot
static_assert(TestParseError<Config_double>(R"({"value":.42})", JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER));   // Leading dot
static_assert(TestParseError<Config_double>(R"({"value":1e})", JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER));     // Incomplete exponent
static_assert(TestParseError<Config_double>(R"({"value":00.5})", JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER));  // Leading zeros

// ============================================================================
// Edge Cases
// ============================================================================

// Number at start of array
struct Config_array { std::array<int, 3> values; };
static_assert(TestParse(R"({"values":[0,1,2]})", Config_array{{0, 1, 2}}));
static_assert(TestParse(R"({"values":[-1,-2,-3]})", Config_array{{-1, -2, -3}}));

// Multiple number fields
struct Config_multi {
    int a;
    int b;
    int c;
};
static_assert(TestParse(R"({"a":0,"b":-0,"c":42})", Config_multi{0, 0, 42}));
static_assert(TestParse(R"({"a":2147483647,"b":-2147483648,"c":0})", Config_multi{2147483647, -2147483648, 0}));

// Numbers with whitespace
static_assert(TestParse(R"({"value": 42 })", Config_int{42}));
static_assert(TestParse(R"({"value":  -123  })", Config_int{-123}));

// ============================================================================
// Roundtrip Tests
// ============================================================================

static_assert(TestRoundTrip<Config_int>(R"({"value":0})", Config_int{0}));
static_assert(TestRoundTrip<Config_int>(R"({"value":42})", Config_int{42}));
static_assert(TestRoundTrip<Config_int>(R"({"value":-123})", Config_int{-123}));
static_assert(TestRoundTrip<Config_int>(R"({"value":2147483647})", Config_int{2147483647}));
static_assert(TestRoundTrip<Config_int>(R"({"value":-2147483648})", Config_int{-2147483648}));

static_assert(TestRoundTrip<Config_int8>(R"({"value":127})", Config_int8{127}));
static_assert(TestRoundTrip<Config_uint8>(R"({"value":255})", Config_uint8{255}));
static_assert(TestRoundTrip<Config_uint32>(R"({"value":4294967295})", Config_uint32{4294967295U}));

static_assert(TestRoundTrip<Config_multi>(R"({"a":0,"b":0,"c":42})", Config_multi{0, 0, 42}));

