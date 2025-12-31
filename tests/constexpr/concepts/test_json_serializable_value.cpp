#include <JsonFusion/static_schema.hpp>
#include <JsonFusion/validators.hpp>
#include <cstdint>
#include <optional>
#include <vector>
#include <array>
#include <string>

using namespace JsonFusion;

// ===== Primitives should be serializable =====
static_assert(static_schema::SerializableValue<bool>);
static_assert(static_schema::SerializableValue<int>);
static_assert(static_schema::SerializableValue<int8_t>);
static_assert(static_schema::SerializableValue<int16_t>);
static_assert(static_schema::SerializableValue<int32_t>);
static_assert(static_schema::SerializableValue<int64_t>);
static_assert(static_schema::SerializableValue<uint8_t>);
static_assert(static_schema::SerializableValue<uint16_t>);
static_assert(static_schema::SerializableValue<uint32_t>);
static_assert(static_schema::SerializableValue<uint64_t>);
static_assert(static_schema::SerializableValue<float>);
static_assert(static_schema::SerializableValue<double>);

// ===== Strings should be serializable =====
static_assert(static_schema::SerializableValue<std::string>);
static_assert(static_schema::SerializableValue<std::array<char, 32>>);
static_assert(static_schema::SerializableValue<std::vector<char>>);

// ===== Aggregates (structs) should be serializable =====
struct SimpleStruct {
    int x;
    bool flag;
};
static_assert(static_schema::SerializableValue<SimpleStruct>);

struct NestedStruct {
    int id;
    SimpleStruct inner;
};
static_assert(static_schema::SerializableValue<NestedStruct>);

// ===== Arrays/containers should be serializable =====
static_assert(static_schema::SerializableValue<std::vector<int>>);
static_assert(static_schema::SerializableValue<std::array<int, 10>>);
static_assert(static_schema::SerializableValue<std::vector<SimpleStruct>>);

// ===== Optionals of serializable types should be serializable =====
static_assert(static_schema::SerializableValue<std::optional<int>>);
static_assert(static_schema::SerializableValue<std::optional<bool>>);
static_assert(static_schema::SerializableValue<std::optional<std::string>>);
static_assert(static_schema::SerializableValue<std::optional<SimpleStruct>>);
static_assert(static_schema::SerializableValue<std::optional<std::vector<int>>>);

// ===== Annotated types should be serializable =====
static_assert(static_schema::SerializableValue<Annotated<int, validators::range<0, 100>>>);
static_assert(static_schema::SerializableValue<Annotated<std::string, validators::min_length<1>>>);

// ===== Things that should NOT be serializable =====

// Pointers
static_assert(!static_schema::SerializableValue<int*>);
static_assert(!static_schema::SerializableValue<SimpleStruct*>);
static_assert(!static_schema::SerializableValue<void*>);
static_assert(!static_schema::SerializableValue<std::nullptr_t>);

// Member pointers
static_assert(!static_schema::SerializableValue<int SimpleStruct::*>);

// Function types
static_assert(!static_schema::SerializableValue<void()>);
static_assert(!static_schema::SerializableValue<int(int)>);
static_assert(!static_schema::SerializableValue<void(SimpleStruct::*)()>);  // Member function pointer

// Void
static_assert(!static_schema::SerializableValue<void>);

// Nested optionals
static_assert(!static_schema::SerializableValue<std::optional<std::optional<int>>>);

// Optional of non-serializable type
struct NonAggregate {
    NonAggregate() {}  // Constructor makes it non-aggregate
    int x;
};
static_assert(!static_schema::SerializableValue<std::optional<NonAggregate>>);

// Containers of non-serializable types
static_assert(!static_schema::SerializableValue<std::vector<int*>>);
static_assert(!static_schema::SerializableValue<std::vector<void*>>);

