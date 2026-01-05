#include <iostream>
#include "JsonFusion/wire_sink.hpp"
#include "JsonFusion/static_schema.hpp"

using namespace JsonFusion;



static_assert(WireSinkLike<WireSink<1024>>, "WireSink should satisfy WireSinkLike");
static_assert(WireSinkLike<WireSink<1024, true>>, "Dynamic WireSink should satisfy WireSinkLike");

// Test that WireSink is recognized as ParsableValue and SerializableValue
static_assert(static_schema::ParsableValue<WireSink<1024>>, "WireSink should be ParsableValue");
static_assert(static_schema::SerializableValue<WireSink<1024>>, "WireSink should be SerializableValue");

// Test that WireSink is NOT recognized as other types
static_assert(!static_schema::BoolLike<WireSink<1024>>, "WireSink should NOT be BoolLike");
static_assert(!static_schema::StringLike<WireSink<1024>>, "WireSink should NOT be StringLike");
static_assert(!static_schema::NumberLike<WireSink<1024>>, "WireSink should NOT be NumberLike");
static_assert(!static_schema::ObjectLike<WireSink<1024>>, "WireSink should NOT be ObjectLike");
static_assert(!static_schema::ParsableArrayLike<WireSink<1024>>, "WireSink should NOT be ParsableArrayLike");
static_assert(!static_schema::SerializableArrayLike<WireSink<1024>>, "WireSink should NOT be SerializableArrayLike");
static_assert(!static_schema::ParsableMapLike<WireSink<1024>>, "WireSink should NOT be ParsableMapLike");
static_assert(!static_schema::SerializableMapLike<WireSink<1024>>, "WireSink should NOT be SerializableMapLike");