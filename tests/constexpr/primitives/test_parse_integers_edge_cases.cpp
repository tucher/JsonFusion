#include "test_helpers.hpp"
using namespace TestHelpers;

struct Config { int value; };

// ===== Whitespace around numbers =====
static_assert(TestParse(R"({"value":  42  })", Config{42}));
static_assert(TestParse(R"({"value": 42})", Config{42}));

// ===== Negative zero =====
static_assert(TestParse(R"({"value": -0})", Config{0}));

// ===== Multiple digits =====
static_assert(TestParse(R"({"value": 1234567890})", Config{1234567890}));
static_assert(TestParse(R"({"value": -1234567890})", Config{-1234567890}));

// ===== Leading zeros =====
// Note: JsonFusion accepts leading zeros (some parsers reject them per JSON spec)
static_assert(TestParse(R"({"value": 007})", Config{7}));

// ===== Error: String where int expected =====
static_assert(TestParseError<Config>(R"({"value": "42"})", JsonFusion::ParseError::ILLFORMED_NUMBER));

// ===== Error: Float where int expected =====
static_assert(TestParseError<Config>(R"({"value": 42.5})", JsonFusion::ParseError::ILLFORMED_NUMBER));

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

