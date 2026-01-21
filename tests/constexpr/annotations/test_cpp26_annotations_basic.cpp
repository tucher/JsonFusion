// ============================================================================
// C++26 Annotations Basic Test
// ============================================================================
// Compile with: ~/gcc_latest/install/bin/g++ -std=c++26 -freflection \
//               -I/Users/tucher/JsonFusion/include \
//               tests/constexpr/annotations/test_cpp26_annotations_basic.cpp
//
// This test verifies that C++26 annotations (via [[=OptionsPack<...>{}]])
// are correctly extracted and used for validation during JSON parsing.
//
// NOTE: This test is skipped (compiles to empty) when C++26 reflection is not available.
// ============================================================================

// Check for C++26 reflection support
#include <JsonFusion/struct_introspection.hpp>

#if JSONFUSION_USE_REFLECTION
// ============================================================================
// C++26 REFLECTION AVAILABLE - Run all tests
// ============================================================================

#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>

using namespace JsonFusion;
using namespace JsonFusion::validators;

// ============================================================================
// Test: Basic range validation with C++26 annotations
// ============================================================================

// Instead of: Annotated<int, range<0, 100>> value;
// We use:     [[=OptionsPack<range<0, 100>>{}]] int value;

// Test: range<0, 100> accepts value at min boundary
constexpr bool test_annotation_range_min_boundary_valid() {
    struct Test {
        [[=OptionsPack<range<0, 100>>{}]] int value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0})";
    auto result = Parse(obj, json);
    
    return result && obj.value == 0;
}
static_assert(test_annotation_range_min_boundary_valid(), 
    "[[=OptionsPack<range<0, 100>>{}]] accepts min boundary (0)");

// Test: range<0, 100> accepts value at max boundary
constexpr bool test_annotation_range_max_boundary_valid() {
    struct Test {
        [[=OptionsPack<range<0, 100>>{}]] int value;
    };
    
    Test obj{};
    std::string json = R"({"value": 100})";
    auto result = Parse(obj, json);
    
    return result && obj.value == 100;
}
static_assert(test_annotation_range_max_boundary_valid(), 
    "[[=OptionsPack<range<0, 100>>{}]] accepts max boundary (100)");

// Test: range<0, 100> accepts value in middle
constexpr bool test_annotation_range_middle_valid() {
    struct Test {
        [[=OptionsPack<range<0, 100>>{}]] int value;
    };
    
    Test obj{};
    std::string json = R"({"value": 50})";
    auto result = Parse(obj, json);
    
    return result && obj.value == 50;
}
static_assert(test_annotation_range_middle_valid(), 
    "[[=OptionsPack<range<0, 100>>{}]] accepts middle value (50)");

// ============================================================================
// Test: range<> - Invalid Values (Below Min, Above Max)
// ============================================================================

// Test: range<0, 100> rejects value below min
constexpr bool test_annotation_range_below_min() {
    struct Test {
        [[=OptionsPack<range<0, 100>>{}]] int value;
    };
    
    Test obj{};
    std::string json = R"({"value": -1})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_annotation_range_below_min(), 
    "[[=OptionsPack<range<0, 100>>{}]] rejects value below min (-1)");

// Test: range<0, 100> rejects value above max
constexpr bool test_annotation_range_above_max() {
    struct Test {
        [[=OptionsPack<range<0, 100>>{}]] int value;
    };
    
    Test obj{};
    std::string json = R"({"value": 101})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_annotation_range_above_max(), 
    "[[=OptionsPack<range<0, 100>>{}]] rejects value above max (101)");

// ============================================================================
// Test: Multiple annotated fields
// ============================================================================

constexpr bool test_annotation_multiple_fields() {
    struct Config {
        [[=OptionsPack<range<0, 65535>>{}]] int port;
        [[=OptionsPack<range<1, 100>>{}]] int max_connections;
        int plain_field;  // No annotation - no validation
    };
    
    Config obj{};
    std::string json = R"({"port": 8080, "max_connections": 50, "plain_field": 999999})";
    auto result = Parse(obj, json);
    
    return result 
        && obj.port == 8080 
        && obj.max_connections == 50
        && obj.plain_field == 999999;
}
static_assert(test_annotation_multiple_fields(), 
    "Multiple annotated fields with range validation");

constexpr bool test_annotation_multiple_fields_one_fails() {
    struct Config {
        [[=OptionsPack<range<0, 65535>>{}]] int port;
        [[=OptionsPack<range<1, 100>>{}]] int max_connections;
    };
    
    Config obj{};
    std::string json = R"({"port": 8080, "max_connections": 150})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_annotation_multiple_fields_one_fails(), 
    "Multiple annotated fields - one fails validation");

// ============================================================================
// Test: Negative range values
// ============================================================================

constexpr bool test_annotation_negative_range() {
    struct Test {
        [[=OptionsPack<range<-100, -10>>{}]] int value;
    };
    
    Test obj{};
    std::string json = R"({"value": -50})";
    auto result = Parse(obj, json);
    
    return result && obj.value == -50;
}
static_assert(test_annotation_negative_range(), 
    "[[=OptionsPack<range<-100, -10>>{}]] accepts value in negative range");

constexpr bool test_annotation_negative_range_rejects_positive() {
    struct Test {
        [[=OptionsPack<range<-100, -10>>{}]] int value;
    };
    
    Test obj{};
    std::string json = R"({"value": 5})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_annotation_negative_range_rejects_positive(), 
    "[[=OptionsPack<range<-100, -10>>{}]] rejects positive value");

// ============================================================================
// Test: Single-value range
// ============================================================================

constexpr bool test_annotation_single_value_range() {
    struct Test {
        [[=OptionsPack<range<42, 42>>{}]] int value;
    };
    
    Test obj{};
    std::string json = R"({"value": 42})";
    auto result = Parse(obj, json);
    
    return result && obj.value == 42;
}
static_assert(test_annotation_single_value_range(), 
    "[[=OptionsPack<range<42, 42>>{}]] accepts exactly 42");

constexpr bool test_annotation_single_value_range_rejects() {
    struct Test {
        [[=OptionsPack<range<42, 42>>{}]] int value;
    };
    
    Test obj{};
    std::string json = R"({"value": 43})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_annotation_single_value_range_rejects(), 
    "[[=OptionsPack<range<42, 42>>{}]] rejects 43");

// ============================================================================
// Test: Field without annotation (no validation)
// ============================================================================

constexpr bool test_annotation_plain_field() {
    struct Test {
        int unrestricted;  // No annotation - any value accepted
    };
    
    Test obj{};
    std::string json = R"({"unrestricted": 999999999})";
    auto result = Parse(obj, json);
    
    return result && obj.unrestricted == 999999999;
}
static_assert(test_annotation_plain_field(), 
    "Field without annotation accepts any value");

// ============================================================================
// Test: Mix of annotated and plain fields
// ============================================================================

constexpr bool test_annotation_mixed_fields() {
    struct Test {
        [[=OptionsPack<range<0, 100>>{}]] int validated;
        int unvalidated;
        [[=OptionsPack<range<-10, 10>>{}]] int also_validated;
    };
    
    Test obj{};
    std::string json = R"({"validated": 50, "unvalidated": 12345, "also_validated": -5})";
    auto result = Parse(obj, json);
    
    return result 
        && obj.validated == 50
        && obj.unvalidated == 12345
        && obj.also_validated == -5;
}
static_assert(test_annotation_mixed_fields(), 
    "Mix of annotated and plain fields works correctly");

// ============================================================================
// Test: Both C++26 annotations and A<>/Annotated<> work simultaneously
// ============================================================================

// Test: Mix of [[=OptionsPack<...>{}]] and Annotated<> in same struct
constexpr bool test_both_syntaxes_valid() {
    struct Config {
        [[=OptionsPack<range<0, 100>>{}]] int new_style;      // C++26 annotation
        Annotated<int, range<0, 100>> old_style;              // Traditional wrapper
        A<int, range<0, 100>> shorthand_style;                // A<> shorthand
        int plain;                                             // No validation
    };
    
    Config obj{};
    std::string json = R"({"new_style": 50, "old_style": 75, "shorthand_style": 25, "plain": 999})";
    auto result = Parse(obj, json);
    
    return result 
        && obj.new_style == 50
        && obj.old_style.get() == 75
        && obj.shorthand_style.get() == 25
        && obj.plain == 999;
}
static_assert(test_both_syntaxes_valid(), 
    "Both [[=OptionsPack<...>{}]] and Annotated<>/A<> work in same struct");

// Test: C++26 annotation fails validation (others valid)
constexpr bool test_both_syntaxes_new_style_fails() {
    struct Config {
        [[=OptionsPack<range<0, 100>>{}]] int new_style;
        Annotated<int, range<0, 100>> old_style;
    };
    
    Config obj{};
    std::string json = R"({"new_style": 150, "old_style": 50})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_both_syntaxes_new_style_fails(), 
    "C++26 annotation validation failure detected in mixed struct");

// Test: Annotated<> fails validation (others valid)
constexpr bool test_both_syntaxes_old_style_fails() {
    struct Config {
        [[=OptionsPack<range<0, 100>>{}]] int new_style;
        Annotated<int, range<0, 100>> old_style;
    };
    
    Config obj{};
    std::string json = R"({"new_style": 50, "old_style": 150})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_both_syntaxes_old_style_fails(), 
    "Annotated<> validation failure detected in mixed struct");

// Test: A<> shorthand fails validation
constexpr bool test_both_syntaxes_shorthand_fails() {
    struct Config {
        [[=OptionsPack<range<0, 100>>{}]] int new_style;
        A<int, range<0, 100>> shorthand;
    };
    
    Config obj{};
    std::string json = R"({"new_style": 50, "shorthand": -5})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_both_syntaxes_shorthand_fails(), 
    "A<> shorthand validation failure detected in mixed struct");

// Test: Complex mixed struct with different validators
constexpr bool test_both_syntaxes_different_validators() {
    struct ServerConfig {
        [[=OptionsPack<range<1, 65535>>{}]] int port;              // C++26: port range
        Annotated<int, range<1, 1000>> max_connections;            // Annotated<>
        A<int, range<1, 3600>> timeout_seconds;                    // A<> shorthand
        [[=OptionsPack<range<0, 100>>{}]] int cpu_threshold;       // C++26: percentage
        int debug_level;                                            // No validation
    };
    
    ServerConfig obj{};
    std::string json = R"({
        "port": 8080,
        "max_connections": 500,
        "timeout_seconds": 300,
        "cpu_threshold": 80,
        "debug_level": 9999
    })";
    auto result = Parse(obj, json);
    
    return result 
        && obj.port == 8080
        && obj.max_connections.get() == 500
        && obj.timeout_seconds.get() == 300
        && obj.cpu_threshold == 80
        && obj.debug_level == 9999;
}
static_assert(test_both_syntaxes_different_validators(), 
    "Complex mixed struct with both syntaxes and different validators");

#endif // JSONFUSION_USE_REFLECTION
