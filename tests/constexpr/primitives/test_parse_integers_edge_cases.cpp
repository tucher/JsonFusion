#include "test_helpers.hpp"
using namespace TestHelpers;

struct Config { int value; };

// ===== Whitespace around numbers =====
static_assert(TestParse(R"({"value":  42  })", Config{42}));
static_assert(TestParse(R"({"value": 42})", Config{42}));

// ===== Multiple digits =====
static_assert(TestParse(R"({"value": 1234567890})", Config{1234567890}));
static_assert(TestParse(R"({"value": -1234567890})", Config{-1234567890}));

// ===== Leading zeros (RFC 8259 violation) =====
// RFC 8259: "Note that leading zeros are not allowed."
static_assert(TestParseError<Config>(R"({"value": 007})", JsonFusion::ParseError::ILLFORMED_NUMBER));
static_assert(TestParseError<Config>(R"({"value": 0123})", JsonFusion::ParseError::ILLFORMED_NUMBER));
static_assert(TestParseError<Config>(R"({"value": -007})", JsonFusion::ParseError::ILLFORMED_NUMBER));

// But "0" by itself is valid
static_assert(TestParse(R"({"value": 0})", Config{0}));
static_assert(TestParse(R"({"value": -0})", Config{0}));  // Also valid

// ===== Error: String where int expected =====
static_assert(TestParseError<Config>(R"({"value": "42"})", JsonFusion::ParseError::ILLFORMED_NUMBER));

// ===== Error: Float where int expected =====
static_assert(TestParseError<Config>(R"({"value": 42.5})", JsonFusion::ParseError::FLOAT_VALUE_IN_INTEGER_STORAGE));

// ===== Error: Boolean where int expected =====
static_assert(TestParseError<Config>(R"({"value": true})", JsonFusion::ParseError::ILLFORMED_NUMBER));

// ===== Error: Null where int expected =====
static_assert(TestParseError<Config>(R"({"value": null})", JsonFusion::ParseError::NULL_IN_NON_OPTIONAL));

// ===== Error: Array where int expected =====
static_assert(TestParseError<Config>(R"({"value": [42]})", JsonFusion::ParseError::ILLFORMED_NUMBER));

// ===== Error: Object where int expected =====
static_assert(TestParseError<Config>(R"({"value": {"x": 42}})", JsonFusion::ParseError::ILLFORMED_NUMBER));

// ===== Error: Invalid characters in number =====
static_assert(TestParseError<Config>(R"({"value": 42a})", JsonFusion::ParseError::ILLFORMED_NUMBER));
static_assert(TestParseError<Config>(R"({"value": 4.2.3})", JsonFusion::ParseError::ILLFORMED_NUMBER));

