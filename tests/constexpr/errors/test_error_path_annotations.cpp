#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <array>

using namespace TestHelpers;
using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace JsonFusion::validators;

// ============================================================================
// Test 1: Basic struct with key<> annotation
// ============================================================================

struct SimpleConfig {
    A<int, key<"id">> identifier;
};

// Test parsing succeeds
static_assert([]() constexpr {
    SimpleConfig cfg{};
    return Parse(cfg, std::string_view(R"({"id": 42})"));
}(), "Test 1: key<> annotation - successful parse");

// ============================================================================
// Test 2: Error detection with key<> annotation
// ============================================================================

// Test error is detected when type is wrong
static_assert([]() constexpr {
    SimpleConfig cfg{};
    auto result = Parse(cfg, std::string_view(R"({"id": "bad"})"));
    return !result && result.readerError() == JsonIteratorReaderError::ILLFORMED_NUMBER;
}(), "Test 2: key<> annotation - error detection");

// ============================================================================
// Test 3: exclude annotation - field skipped in JSON
// ============================================================================

struct WithHiddenField {
    int visible;
    A<int, exclude> hidden;
};

static_assert([]() constexpr {
    WithHiddenField obj{};
    // Only "visible" in JSON, "hidden" is exclude
    return Parse(obj, std::string_view(R"({"visible": 42})"));
}(), "Test 3: exclude - field skipped");

// ============================================================================
// Test 4: Nested structs with key<> annotations
// ============================================================================

struct Outer {
    A<int, key<"outer-id">> id;
    struct Inner {
        A<int, key<"inner-value">> value;
    };
    A<Inner, key<"inner-data">> data;
};

static_assert([]() constexpr {
    Outer obj{};
    return Parse(obj, std::string_view(R"({
        "outer-id": 1,
        "inner-data": {"inner-value": 42}
    })"));
}(), "Test 4: Nested key<> annotations");

// ============================================================================
// Test 5: Multiple fields with key<> annotations
// ============================================================================

struct MultiField {
    A<int, key<"field-one">> f1;
    A<int, key<"field-two">> f2;
    A<int, key<"field-three">> f3;
};

static_assert([]() constexpr {
    MultiField obj{};
    return Parse(obj, std::string_view(R"({
        "field-one": 1,
        "field-two": 2,
        "field-three": 3
    })"));
}(), "Test 5: Multiple key<> annotations");

// ============================================================================
// Test 6: key<> with validation
// ============================================================================

struct ValidatedWithKey {
    A<int, key<"port">, range<1, 65535>> portNumber;
};

static_assert([]() constexpr {
    ValidatedWithKey obj{};
    return Parse(obj, std::string_view(R"({"port": 8080})"));
}(), "Test 6: key<> with validation - success");

static_assert([]() constexpr {
    ValidatedWithKey obj{};
    auto result = Parse(obj, std::string_view(R"({"port": 999999})"));
    return !result && !static_cast<bool>(result.validationErrors());
}(), "Test 7: key<> with validation - error detected");
