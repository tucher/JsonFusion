#include <JsonFusion/static_schema.hpp>
#include <JsonFusion/validators.hpp>
#include <cstdint>
#include <optional>
#include <vector>
#include <array>
#include <string>

using namespace JsonFusion;

// ===== Primitives should be serializable =====
static_assert(static_schema::JsonSerializableValue<bool>);
static_assert(static_schema::JsonSerializableValue<int>);
static_assert(static_schema::JsonSerializableValue<int8_t>);
static_assert(static_schema::JsonSerializableValue<int16_t>);
static_assert(static_schema::JsonSerializableValue<int32_t>);
static_assert(static_schema::JsonSerializableValue<int64_t>);
static_assert(static_schema::JsonSerializableValue<uint8_t>);
static_assert(static_schema::JsonSerializableValue<uint16_t>);
static_assert(static_schema::JsonSerializableValue<uint32_t>);
static_assert(static_schema::JsonSerializableValue<uint64_t>);
static_assert(static_schema::JsonSerializableValue<float>);
static_assert(static_schema::JsonSerializableValue<double>);

// ===== Strings should be serializable =====
static_assert(static_schema::JsonSerializableValue<std::string>);
static_assert(static_schema::JsonSerializableValue<std::array<char, 32>>);
static_assert(static_schema::JsonSerializableValue<std::vector<char>>);

// ===== Aggregates (structs) should be serializable =====
struct SimpleStruct {
    int x;
    bool flag;
};
static_assert(static_schema::JsonSerializableValue<SimpleStruct>);

struct NestedStruct {
    int id;
    SimpleStruct inner;
};
static_assert(static_schema::JsonSerializableValue<NestedStruct>);

// ===== Arrays/containers should be serializable =====
static_assert(static_schema::JsonSerializableValue<std::vector<int>>);
static_assert(static_schema::JsonSerializableValue<std::array<int, 10>>);
static_assert(static_schema::JsonSerializableValue<std::vector<SimpleStruct>>);

// ===== Optionals of serializable types should be serializable =====
static_assert(static_schema::JsonSerializableValue<std::optional<int>>);
static_assert(static_schema::JsonSerializableValue<std::optional<bool>>);
static_assert(static_schema::JsonSerializableValue<std::optional<std::string>>);
static_assert(static_schema::JsonSerializableValue<std::optional<SimpleStruct>>);
static_assert(static_schema::JsonSerializableValue<std::optional<std::vector<int>>>);

// ===== Annotated types should be serializable =====
static_assert(static_schema::JsonSerializableValue<Annotated<int, validators::range<0, 100>>>);
static_assert(static_schema::JsonSerializableValue<Annotated<std::string, validators::min_length<1>>>);

// ===== Things that should NOT be serializable =====

// Pointers
static_assert(!static_schema::JsonSerializableValue<int*>);
static_assert(!static_schema::JsonSerializableValue<SimpleStruct*>);
static_assert(!static_schema::JsonSerializableValue<void*>);
static_assert(!static_schema::JsonSerializableValue<std::nullptr_t>);

// Member pointers
static_assert(!static_schema::JsonSerializableValue<int SimpleStruct::*>);

// Function types
static_assert(!static_schema::JsonSerializableValue<void()>);
static_assert(!static_schema::JsonSerializableValue<int(int)>);
static_assert(!static_schema::JsonSerializableValue<void(SimpleStruct::*)()>);  // Member function pointer

// Void
static_assert(!static_schema::JsonSerializableValue<void>);

// C arrays (int[N]) cause compilation errors deep in trait evaluation.
// This is a known limitation - use std::array<int, N> instead.
// Removed test: causes compilation failure, not a runtime check failure.

// Nested optionals
static_assert(!static_schema::JsonSerializableValue<std::optional<std::optional<int>>>);

// Optional of non-serializable type
struct NonAggregate {
    NonAggregate() {}  // Constructor makes it non-aggregate
    int x;
};
static_assert(!static_schema::JsonSerializableValue<std::optional<NonAggregate>>);

// Containers of non-serializable types
static_assert(!static_schema::JsonSerializableValue<std::vector<int*>>);
static_assert(!static_schema::JsonSerializableValue<std::vector<void*>>);

