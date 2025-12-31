#include "../test_helpers.hpp"
#include <JsonFusion/wire_sink.hpp>
#include <JsonFusion/static_schema.hpp>
#include <JsonFusion/options.hpp>

using namespace JsonFusion;
using namespace JsonFusion::static_schema;
using namespace JsonFusion::options;

// ============================================================================
// Test: WireSink Integration with Static Schema
// ============================================================================

// Test: WireSink is recognized as parsable
static_assert(is_json_parsable_value<WireSink<256>>::value, 
              "WireSink should be parsable");
static_assert(is_json_parsable_value<WireSink<1024, true>>::value, 
              "Dynamic WireSink should be parsable");

// Test: WireSink is recognized as serializable
static_assert(is_json_serializable_value<WireSink<256>>::value, 
              "WireSink should be serializable");
static_assert(is_json_serializable_value<WireSink<1024, true>>::value, 
              "Dynamic WireSink should be serializable");

// Test: WireSink satisfies ParsableValue concept
static_assert(ParsableValue<WireSink<256>>, 
              "WireSink should satisfy ParsableValue concept");
static_assert(ParsableValue<WireSink<1024, true>>, 
              "Dynamic WireSink should satisfy ParsableValue concept");

// Test: WireSink satisfies SerializableValue concept
static_assert(SerializableValue<WireSink<256>>, 
              "WireSink should satisfy SerializableValue concept");
static_assert(SerializableValue<WireSink<1024, true>>, 
              "Dynamic WireSink should satisfy SerializableValue concept");

// Test: WireSink is NOT recognized as other types
static_assert(!StringLike<WireSink<256>>, 
              "WireSink should NOT be StringLike");
static_assert(!is_json_object<WireSink<256>>::value, 
              "WireSink should NOT be a JSON object");
static_assert(!is_json_parsable_array<WireSink<256>>::value, 
              "WireSink should NOT be a parsable array");
static_assert(!is_json_serializable_array<WireSink<256>>::value, 
              "WireSink should NOT be a serializable array");
static_assert(!is_json_parsable_map<WireSink<256>>::value, 
              "WireSink should NOT be a parsable map");
static_assert(!is_json_serializable_map<WireSink<256>>::value, 
              "WireSink should NOT be a serializable map");
static_assert(!is_non_null_json_parsable_value<WireSink<256>>::value, 
              "WireSink should NOT be a regular non-null parsable value");
static_assert(!is_non_null_json_serializable_value<WireSink<256>>::value, 
              "WireSink should NOT be a regular non-null serializable value");

// Test: WireSink works in structs
struct MessageWithWireSink {
    int id;
    WireSink<1024> payload;
};

static_assert(is_json_object<MessageWithWireSink>::value, 
              "Struct with WireSink should be a JSON object");
static_assert(ParsableValue<MessageWithWireSink>, 
              "Struct with WireSink should be parsable");
static_assert(SerializableValue<MessageWithWireSink>, 
              "Struct with WireSink should be serializable");

// Test: WireSink with Annotated wrapper
struct MessageWithAnnotatedWireSink {
    int id;
    Annotated<WireSink<512>, key<"data">> payload;
};

static_assert(is_json_object<MessageWithAnnotatedWireSink>::value, 
              "Struct with Annotated WireSink should be a JSON object");
static_assert(ParsableValue<MessageWithAnnotatedWireSink>, 
              "Struct with Annotated WireSink should be parsable");
static_assert(SerializableValue<MessageWithAnnotatedWireSink>, 
              "Struct with Annotated WireSink should be serializable");

// Test: Multiple WireSinks in struct
struct MultiSinkMessage {
    WireSink<256> header;
    WireSink<1024> body;
    WireSink<128> footer;
};

static_assert(is_json_object<MultiSinkMessage>::value, 
              "Struct with multiple WireSinks should be a JSON object");
static_assert(ParsableValue<MultiSinkMessage>, 
              "Struct with multiple WireSinks should be parsable");
static_assert(SerializableValue<MultiSinkMessage>, 
              "Struct with multiple WireSinks should be serializable");

// Test: WireSink with other types
struct MixedMessage {
    std::string type;
    int count;
    WireSink<2048, true> data;  // Dynamic
    bool flag;
};

static_assert(is_json_object<MixedMessage>::value, 
              "Struct mixing WireSink with other types should be a JSON object");
static_assert(ParsableValue<MixedMessage>, 
              "Struct mixing WireSink with other types should be parsable");
static_assert(SerializableValue<MixedMessage>, 
              "Struct mixing WireSink with other types should be serializable");

// Compile-time success message
constexpr bool all_tests_passed = true;
static_assert(all_tests_passed, "All WireSink static schema tests passed!");

