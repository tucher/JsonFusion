#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>

using namespace JsonFusion;
using namespace JsonFusion::validators;

// Constexpr-compatible absolute value for testing
template<typename T>
constexpr T test_abs(T value) {
    return value < T(0) ? -value : value;
}

// ============================================================================
// Test: range<> - Float - Valid Values at Boundaries
// ============================================================================

// Test: range<0.0f, 100.0f> accepts value at min boundary
constexpr bool test_range_float_min_boundary_valid() {
    struct Test {
        Annotated<float, range<0.0f, 100.0f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0.0})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 0.0f;
}
static_assert(test_range_float_min_boundary_valid(), "range<0.0f, 100.0f> accepts min boundary (0.0)");

// Test: range<0.0f, 100.0f> accepts value at max boundary
constexpr bool test_range_float_max_boundary_valid() {
    struct Test {
        Annotated<float, range<0.0f, 100.0f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 100.0})";
    auto result = Parse(obj, json);
    
    return result && test_abs(obj.value.get() - 100.0f) < 0.001f;
}
static_assert(test_range_float_max_boundary_valid(), "range<0.0f, 100.0f> accepts max boundary (100.0)");

// Test: range<0.0f, 100.0f> accepts value in middle
constexpr bool test_range_float_middle_valid() {
    struct Test {
        Annotated<float, range<0.0f, 100.0f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 50.5})";
    auto result = Parse(obj, json);
    
    return result && test_abs(obj.value.get() - 50.5f) < 0.001f;
}
static_assert(test_range_float_middle_valid(), "range<0.0f, 100.0f> accepts middle value (50.5)");

// ============================================================================
// Test: range<> - Float - Invalid Values (Below Min, Above Max)
// ============================================================================

// Test: range<0.0f, 100.0f> rejects value below min
constexpr bool test_range_float_below_min() {
    struct Test {
        Annotated<float, range<0.0f, 100.0f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -0.1})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_float_below_min(), "range<0.0f, 100.0f> rejects value below min (-0.1)");

// Test: range<0.0f, 100.0f> rejects value above max
constexpr bool test_range_float_above_max() {
    struct Test {
        Annotated<float, range<0.0f, 100.0f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 100.1})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_float_above_max(), "range<0.0f, 100.0f> rejects value above max (100.1)");

// ============================================================================
// Test: range<> - Float - Negative Ranges
// ============================================================================

// Test: range<-100.0f, -10.0f> accepts value at min boundary
constexpr bool test_range_float_negative_min_valid() {
    struct Test {
        Annotated<float, range<-100.0f, -10.0f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -100.0})";
    auto result = Parse(obj, json);
    
    return result && test_abs(obj.value.get() - (-100.0f)) < 0.001f;
}
static_assert(test_range_float_negative_min_valid(), "range<-100.0f, -10.0f> accepts min boundary (-100.0)");

// Test: range<-100.0f, -10.0f> accepts value at max boundary
constexpr bool test_range_float_negative_max_valid() {
    struct Test {
        Annotated<float, range<-100.0f, -10.0f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -10.0})";
    auto result = Parse(obj, json);
    
    return result && test_abs(obj.value.get() - (-10.0f)) < 0.001f;
}
static_assert(test_range_float_negative_max_valid(), "range<-100.0f, -10.0f> accepts max boundary (-10.0)");

// Test: range<-100.0f, -10.0f> rejects value below min
constexpr bool test_range_float_negative_below_min() {
    struct Test {
        Annotated<float, range<-100.0f, -10.0f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -100.1})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_float_negative_below_min(), "range<-100.0f, -10.0f> rejects value below min (-100.1)");

// Test: range<-100.0f, -10.0f> rejects value above max
constexpr bool test_range_float_negative_above_max() {
    struct Test {
        Annotated<float, range<-100.0f, -10.0f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -9.9})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_float_negative_above_max(), "range<-100.0f, -10.0f> rejects value above max (-9.9)");

// ============================================================================
// Test: range<> - Double - Valid Values at Boundaries
// ============================================================================

// Test: range<0.0, 100.0> accepts value at min boundary (double)
constexpr bool test_range_double_min_boundary_valid() {
    struct Test {
        Annotated<double, range<0.0, 100.0>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0.0})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 0.0;
}
static_assert(test_range_double_min_boundary_valid(), "range<0.0, 100.0> accepts min boundary (0.0)");

// Test: range<0.0, 100.0> accepts value at max boundary (double)
constexpr bool test_range_double_max_boundary_valid() {
    struct Test {
        Annotated<double, range<0.0, 100.0>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 100.0})";
    auto result = Parse(obj, json);
    
    return result && test_abs(obj.value.get() - 100.0) < 0.0001;
}
static_assert(test_range_double_max_boundary_valid(), "range<0.0, 100.0> accepts max boundary (100.0)");

// Test: range<0.0, 100.0> accepts value in middle (double)
constexpr bool test_range_double_middle_valid() {
    struct Test {
        Annotated<double, range<0.0, 100.0>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 50.5})";
    auto result = Parse(obj, json);
    
    return result && test_abs(obj.value.get() - 50.5) < 0.0001;
}
static_assert(test_range_double_middle_valid(), "range<0.0, 100.0> accepts middle value (50.5)");

// ============================================================================
// Test: range<> - Double - Invalid Values (Below Min, Above Max)
// ============================================================================

// Test: range<0.0, 100.0> rejects value below min (double)
constexpr bool test_range_double_below_min() {
    struct Test {
        Annotated<double, range<0.0, 100.0>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -0.1})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_double_below_min(), "range<0.0, 100.0> rejects value below min (-0.1)");

// Test: range<0.0, 100.0> rejects value above max (double)
constexpr bool test_range_double_above_max() {
    struct Test {
        Annotated<double, range<0.0, 100.0>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 100.1})";
    auto result = Parse(obj, json);
    
    return !result && result.validationErrors().error() == SchemaError::number_out_of_range;
}
static_assert(test_range_double_above_max(), "range<0.0, 100.0> rejects value above max (100.1)");

// ============================================================================
// Test: range<> - Fractional Precision
// ============================================================================

// Test: range<-1.0f, 1.0f> accepts small fractional values
constexpr bool test_range_float_fractional() {
    struct Test {
        Annotated<float, range<-1.0f, 1.0f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0.123456})";
    auto result = Parse(obj, json);
    
    return result && test_abs(obj.value.get() - 0.123456f) < 0.0001f;
}
static_assert(test_range_float_fractional(), "range<-1.0f, 1.0f> accepts fractional (0.123456)");

// Test: range<-1.0, 1.0> accepts small fractional values (double)
constexpr bool test_range_double_fractional() {
    struct Test {
        Annotated<double, range<-1.0, 1.0>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0.123456789})";
    auto result = Parse(obj, json);
    
    return result && test_abs(obj.value.get() - 0.123456789) < 0.000000001;
}
static_assert(test_range_double_fractional(), "range<-1.0, 1.0> accepts fractional (0.123456789)");

// ============================================================================
// Test: range<> - Zero Crossing Ranges
// ============================================================================

// Test: range<-50.0f, 50.0f> accepts zero
constexpr bool test_range_float_zero_crossing() {
    struct Test {
        Annotated<float, range<-50.0f, 50.0f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 0.0})";
    auto result = Parse(obj, json);
    
    return result && obj.value.get() == 0.0f;
}
static_assert(test_range_float_zero_crossing(), "range<-50.0f, 50.0f> accepts zero");

// Test: range<-50.0f, 50.0f> accepts positive value
constexpr bool test_range_float_zero_crossing_positive() {
    struct Test {
        Annotated<float, range<-50.0f, 50.0f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": 25.5})";
    auto result = Parse(obj, json);
    
    return result && test_abs(obj.value.get() - 25.5f) < 0.001f;
}
static_assert(test_range_float_zero_crossing_positive(), "range<-50.0f, 50.0f> accepts positive (25.5)");

// Test: range<-50.0f, 50.0f> accepts negative value
constexpr bool test_range_float_zero_crossing_negative() {
    struct Test {
        Annotated<float, range<-50.0f, 50.0f>> value;
    };
    
    Test obj{};
    std::string json = R"({"value": -25.5})";
    auto result = Parse(obj, json);
    
    return result && test_abs(obj.value.get() - (-25.5f)) < 0.001f;
}
static_assert(test_range_float_zero_crossing_negative(), "range<-50.0f, 50.0f> accepts negative (-25.5)");


