#include <JsonFusion/static_schema.hpp>
#include <JsonFusion/validators.hpp>
#include <cstdint>
#include <optional>
#include <vector>
#include <array>
#include <string>

using namespace JsonFusion;

// ===== Primitives should be parsable =====
static_assert(static_schema::ParsableValue<bool>);
static_assert(static_schema::ParsableValue<int>);
static_assert(static_schema::ParsableValue<int8_t>);
static_assert(static_schema::ParsableValue<int16_t>);
static_assert(static_schema::ParsableValue<int32_t>);
static_assert(static_schema::ParsableValue<int64_t>);
static_assert(static_schema::ParsableValue<uint8_t>);
static_assert(static_schema::ParsableValue<uint16_t>);
static_assert(static_schema::ParsableValue<uint32_t>);
static_assert(static_schema::ParsableValue<uint64_t>);
static_assert(static_schema::ParsableValue<float>);
static_assert(static_schema::ParsableValue<double>);

// ===== Strings should be parsable =====
static_assert(static_schema::ParsableValue<std::string>);
static_assert(static_schema::ParsableValue<std::array<char, 32>>);
static_assert(static_schema::ParsableValue<std::vector<char>>);

// ===== Aggregates (structs) should be parsable =====
struct SimpleStruct {
    int x;
    bool flag;
};
static_assert(static_schema::ParsableValue<SimpleStruct>);

struct NestedStruct {
    int id;
    SimpleStruct inner;
};
static_assert(static_schema::ParsableValue<NestedStruct>);

// ===== Arrays/containers should be parsable =====
static_assert(static_schema::ParsableValue<std::vector<int>>);
static_assert(static_schema::ParsableValue<std::array<int, 10>>);
static_assert(static_schema::ParsableValue<std::vector<SimpleStruct>>);

// ===== Optionals of parsable types should be parsable =====
static_assert(static_schema::ParsableValue<std::optional<int>>);
static_assert(static_schema::ParsableValue<std::optional<bool>>);
static_assert(static_schema::ParsableValue<std::optional<std::string>>);
static_assert(static_schema::ParsableValue<std::optional<SimpleStruct>>);
static_assert(static_schema::ParsableValue<std::optional<std::vector<int>>>);

// ===== Annotated types should be parsable =====
static_assert(static_schema::ParsableValue<Annotated<int, validators::range<0, 100>>>);
static_assert(static_schema::ParsableValue<Annotated<std::string, validators::min_length<1>>>);

// ===== Things that should NOT be parsable =====

// Pointers
static_assert(!static_schema::ParsableValue<int*>);
static_assert(!static_schema::ParsableValue<SimpleStruct*>);
static_assert(!static_schema::ParsableValue<void*>);
static_assert(!static_schema::ParsableValue<std::nullptr_t>);

// Member pointers
static_assert(!static_schema::ParsableValue<int SimpleStruct::*>);

// Function types
static_assert(!static_schema::ParsableValue<void()>);
static_assert(!static_schema::ParsableValue<int(int)>);
static_assert(!static_schema::ParsableValue<void(SimpleStruct::*)()>);  // Member function pointer

// Void
static_assert(!static_schema::ParsableValue<void>);

// Nested optionals (optional<optional<T>> should not be valid)
static_assert(!static_schema::ParsableValue<std::optional<std::optional<int>>>);

// Optional of non-parsable type (in C++20/C++23 mode)
struct NonAggregate {
    NonAggregate() {}  // Constructor makes it non-aggregate
    int x;
};
#if !JSONFUSION_USE_REFLECTION
// PFR mode: non-aggregates are NOT parsable
static_assert(!static_schema::ParsableValue<std::optional<NonAggregate>>);
#else
// C++26 reflection mode: non-aggregates ARE parsable
static_assert(static_schema::ParsableValue<std::optional<NonAggregate>>);
#endif

// Containers of non-parsable types
static_assert(!static_schema::ParsableValue<std::vector<int*>>);
static_assert(!static_schema::ParsableValue<std::vector<void*>>);

