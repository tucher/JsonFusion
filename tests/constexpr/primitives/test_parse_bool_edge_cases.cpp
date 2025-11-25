#include "test_helpers.hpp"
using namespace TestHelpers;

struct Config { bool flag; };

// ===== Whitespace around booleans =====
static_assert(TestParse(R"({"flag":  true  })", Config{true}));
static_assert(TestParse(R"({"flag":false})", Config{false}));

// ===== Error: Case sensitivity (must be lowercase) =====
static_assert(TestParseError<Config>(R"({"flag": True})", JsonFusion::ParseError::ILLFORMED_BOOL));
static_assert(TestParseError<Config>(R"({"flag": TRUE})", JsonFusion::ParseError::ILLFORMED_BOOL));
static_assert(TestParseError<Config>(R"({"flag": False})", JsonFusion::ParseError::ILLFORMED_BOOL));
static_assert(TestParseError<Config>(R"({"flag": FALSE})", JsonFusion::ParseError::ILLFORMED_BOOL));

// ===== Error: String where bool expected =====
static_assert(TestParseError<Config>(R"({"flag": "true"})", JsonFusion::ParseError::ILLFORMED_BOOL));

// ===== Error: Number where bool expected =====
static_assert(TestParseError<Config>(R"({"flag": 1})", JsonFusion::ParseError::ILLFORMED_BOOL));
static_assert(TestParseError<Config>(R"({"flag": 0})", JsonFusion::ParseError::ILLFORMED_BOOL));

// ===== Error: Null where bool expected =====
static_assert(TestParseError<Config>(R"({"flag": null})", JsonFusion::ParseError::NULL_IN_NON_OPTIONAL));

// ===== Error: Array where bool expected =====
static_assert(TestParseError<Config>(R"({"flag": [true]})", JsonFusion::ParseError::ILLFORMED_BOOL));

// ===== Error: Object where bool expected =====
static_assert(TestParseError<Config>(R"({"flag": {"x": true}})", JsonFusion::ParseError::ILLFORMED_BOOL));

// ===== Error: Typos in boolean =====
static_assert(TestParseError<Config>(R"({"flag": tru})", JsonFusion::ParseError::ILLFORMED_BOOL));
static_assert(TestParseError<Config>(R"({"flag": fals})", JsonFusion::ParseError::ILLFORMED_BOOL));

static_assert(TestParseError<Config>(R"({"flag": truee})", JsonFusion::ParseError::ILLFORMED_BOOL));
static_assert(TestParseError<Config>(R"({"flag": falsee})", JsonFusion::ParseError::ILLFORMED_BOOL));

