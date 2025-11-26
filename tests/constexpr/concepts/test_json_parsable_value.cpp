#include <JsonFusion/static_schema.hpp>
#include <JsonFusion/validators.hpp>
#include <cstdint>
#include <optional>
#include <vector>
#include <array>
#include <string>

using namespace JsonFusion;

// ===== Primitives should be parsable =====
static_assert(static_schema::JsonParsableValue<bool>);
static_assert(static_schema::JsonParsableValue<int>);
static_assert(static_schema::JsonParsableValue<int8_t>);
static_assert(static_schema::JsonParsableValue<int16_t>);
static_assert(static_schema::JsonParsableValue<int32_t>);
static_assert(static_schema::JsonParsableValue<int64_t>);
static_assert(static_schema::JsonParsableValue<uint8_t>);
static_assert(static_schema::JsonParsableValue<uint16_t>);
static_assert(static_schema::JsonParsableValue<uint32_t>);
static_assert(static_schema::JsonParsableValue<uint64_t>);
static_assert(static_schema::JsonParsableValue<float>);
static_assert(static_schema::JsonParsableValue<double>);

// ===== Strings should be parsable =====
static_assert(static_schema::JsonParsableValue<std::string>);
static_assert(static_schema::JsonParsableValue<std::array<char, 32>>);
static_assert(static_schema::JsonParsableValue<std::vector<char>>);

// ===== Aggregates (structs) should be parsable =====
struct SimpleStruct {
    int x;
    bool flag;
};
static_assert(static_schema::JsonParsableValue<SimpleStruct>);

struct NestedStruct {
    int id;
    SimpleStruct inner;
};
static_assert(static_schema::JsonParsableValue<NestedStruct>);

// ===== Arrays/containers should be parsable =====
static_assert(static_schema::JsonParsableValue<std::vector<int>>);
static_assert(static_schema::JsonParsableValue<std::array<int, 10>>);
static_assert(static_schema::JsonParsableValue<std::vector<SimpleStruct>>);

// ===== Optionals of parsable types should be parsable =====
static_assert(static_schema::JsonParsableValue<std::optional<int>>);
static_assert(static_schema::JsonParsableValue<std::optional<bool>>);
static_assert(static_schema::JsonParsableValue<std::optional<std::string>>);
static_assert(static_schema::JsonParsableValue<std::optional<SimpleStruct>>);
static_assert(static_schema::JsonParsableValue<std::optional<std::vector<int>>>);

// ===== Annotated types should be parsable =====
static_assert(static_schema::JsonParsableValue<Annotated<int, validators::range<0, 100>>>);
static_assert(static_schema::JsonParsableValue<Annotated<std::string, validators::min_length<1>>>);

// ===== Things that should NOT be parsable =====

// Pointers
static_assert(!static_schema::JsonParsableValue<int*>);
static_assert(!static_schema::JsonParsableValue<SimpleStruct*>);
static_assert(!static_schema::JsonParsableValue<void*>);
static_assert(!static_schema::JsonParsableValue<std::nullptr_t>);

// Member pointers
static_assert(!static_schema::JsonParsableValue<int SimpleStruct::*>);

// Function types
static_assert(!static_schema::JsonParsableValue<void()>);
static_assert(!static_schema::JsonParsableValue<int(int)>);
static_assert(!static_schema::JsonParsableValue<void(SimpleStruct::*)()>);  // Member function pointer

// Void
static_assert(!static_schema::JsonParsableValue<void>);

// Nested optionals (optional<optional<T>> should not be valid)
static_assert(!static_schema::JsonParsableValue<std::optional<std::optional<int>>>);

// Optional of non-parsable type
struct NonAggregate {
    NonAggregate() {}  // Constructor makes it non-aggregate
    int x;
};
static_assert(!static_schema::JsonParsableValue<std::optional<NonAggregate>>);

// Containers of non-parsable types
static_assert(!static_schema::JsonParsableValue<std::vector<int*>>);
static_assert(!static_schema::JsonParsableValue<std::vector<void*>>);

