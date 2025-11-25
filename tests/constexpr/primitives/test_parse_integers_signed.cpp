#include "test_helpers.hpp"
using namespace TestHelpers;
#include <cstdint>

// ===== int8_t =====
struct ConfigInt8 { int8_t value; };

static_assert(TestParse(R"({"value": 0})", ConfigInt8{0}));
static_assert(TestParse(R"({"value": 127})", ConfigInt8{127}));    // INT8_MAX
static_assert(TestParse(R"({"value": -128})", ConfigInt8{-128}));  // INT8_MIN
static_assert(TestParse(R"({"value": 42})", ConfigInt8{42}));
static_assert(TestParse(R"({"value": -42})", ConfigInt8{-42}));

// ===== int16_t =====
struct ConfigInt16 { int16_t value; };

static_assert(TestParse(R"({"value": 0})", ConfigInt16{0}));
static_assert(TestParse(R"({"value": 32767})", ConfigInt16{32767}));     // INT16_MAX
static_assert(TestParse(R"({"value": -32768})", ConfigInt16{-32768}));   // INT16_MIN
static_assert(TestParse(R"({"value": 1234})", ConfigInt16{1234}));
static_assert(TestParse(R"({"value": -1234})", ConfigInt16{-1234}));

// ===== int32_t =====
struct ConfigInt32 { int32_t value; };

static_assert(TestParse(R"({"value": 0})", ConfigInt32{0}));
static_assert(TestParse(R"({"value": 2147483647})", ConfigInt32{2147483647}));     // INT32_MAX
static_assert(TestParse(R"({"value": -2147483648})", ConfigInt32{-2147483648}));   // INT32_MIN
static_assert(TestParse(R"({"value": 123456})", ConfigInt32{123456}));
static_assert(TestParse(R"({"value": -123456})", ConfigInt32{-123456}));

// ===== int64_t =====
struct ConfigInt64 { int64_t value; };

static_assert(TestParse(R"({"value": 0})", ConfigInt64{0}));
static_assert(TestParse(R"({"value": 9223372036854775807})", ConfigInt64{9223372036854775807LL}));    // INT64_MAX
static_assert(TestParse(R"({"value": -9223372036854775808})", ConfigInt64{-9223372036854775807LL-1})); // INT64_MIN
static_assert(TestParse(R"({"value": 1234567890})", ConfigInt64{1234567890}));
static_assert(TestParse(R"({"value": -1234567890})", ConfigInt64{-1234567890}));

