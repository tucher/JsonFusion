#include "test_helpers.hpp"
#include <string_view>

using namespace TestHelpers;

// Demo struct for testing
struct Config {
    int value;
    bool flag;
};

// ============================================================================
// Positive Tests - Using ParseSucceeds
// ============================================================================

static_assert([]() constexpr {
    Config c;
    return ParseSucceeds(c, std::string_view(R"({"value": 42, "flag": true})"))
        && c.value == 42
        && c.flag == true;
}(), "Should parse valid JSON");

// ============================================================================
// Negative Tests - Using ParseFails (any error)
// ============================================================================

static_assert([]() constexpr {
    Config c;
    // Missing closing brace - should fail
    return ParseFails(c, std::string_view(R"({"value": 42)"));
}(), "Should fail on malformed JSON");

// ============================================================================
// Specific Error Code Tests - Using ParseFailsWith
// ============================================================================

static_assert([]() constexpr {
    Config c;
    // String where int expected -> ILLFORMED_NUMBER
    return ParseFailsWith(c, 
        std::string_view(R"({"value": "not_a_number", "flag": true})"),
        JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER);
}(), "Should fail with ILLFORMED_NUMBER error");

static_assert([]() constexpr {
    Config c;
    // Unclosed object
    return ParseFailsWith(c, 
        std::string_view(R"({"value": 42, "flag": true)"),
        JsonFusion::JsonIteratorReaderError::ILLFORMED_OBJECT);
}(), "Should fail with ILLFORMED_OBJECT error");

// ============================================================================
// Error Position Tests - Using ParseFailsAt
// ============================================================================

static_assert([]() constexpr {
    Config c;
    // Error at position 10 (where "not_a_number" starts)
    //                    0123456789012345678
    return ParseFailsAt(c,
        std::string_view(R"({"value": "not_a_number"})"),
        JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER,
        10);  // Approximate position of the error
}(), "Should fail at correct position");

// ============================================================================
// Using TestParseError for one-line error tests
// ============================================================================

static_assert(TestParseError<Config>(
    R"({"value": "string"})", 
    JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER));

