#include "test_helpers.hpp"
using namespace TestHelpers;
#include <cstdint>

// ===== uint8_t =====
struct ConfigUInt8 { uint8_t value; };

static_assert(TestParse(R"({"value": 0})", ConfigUInt8{0}));
static_assert(TestParse(R"({"value": 255})", ConfigUInt8{255}));  // UINT8_MAX
static_assert(TestParse(R"({"value": 42})", ConfigUInt8{42}));
static_assert(TestParse(R"({"value": 128})", ConfigUInt8{128}));

// ===== uint16_t =====
struct ConfigUInt16 { uint16_t value; };

static_assert(TestParse(R"({"value": 0})", ConfigUInt16{0}));
static_assert(TestParse(R"({"value": 65535})", ConfigUInt16{65535}));  // UINT16_MAX
static_assert(TestParse(R"({"value": 1234})", ConfigUInt16{1234}));
static_assert(TestParse(R"({"value": 32768})", ConfigUInt16{32768}));

// ===== uint32_t =====
struct ConfigUInt32 { uint32_t value; };

static_assert(TestParse(R"({"value": 0})", ConfigUInt32{0}));
static_assert(TestParse(R"({"value": 4294967295})", ConfigUInt32{4294967295U}));  // UINT32_MAX
static_assert(TestParse(R"({"value": 123456})", ConfigUInt32{123456}));
static_assert(TestParse(R"({"value": 2147483648})", ConfigUInt32{2147483648U}));

// ===== uint64_t =====
struct ConfigUInt64 { uint64_t value; };

static_assert(TestParse(R"({"value": 0})", ConfigUInt64{0}));
static_assert(TestParse(R"({"value": 18446744073709551615})", ConfigUInt64{18446744073709551615ULL})); // UINT64_MAX
static_assert(TestParse(R"({"value": 1234567890})", ConfigUInt64{1234567890}));
static_assert(TestParse(R"({"value": 9223372036854775808})", ConfigUInt64{9223372036854775808ULL}));

