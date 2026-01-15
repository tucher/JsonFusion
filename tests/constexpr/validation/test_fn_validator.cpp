#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/annotated.hpp>
#include <JsonFusion/validators.hpp>
#include <array>
#include <optional>

using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace JsonFusion::validators;
namespace events = validators_detail::parsing_events_tags;

// ============================================================================
// fn_validator allows custom validation logic on parsing events
// ============================================================================

// ============================================================================
// Test: fn_validator on number_parsing_finished (divisibility check)
// ============================================================================

// Divisibility by 10 check
constexpr auto div_by_10 = [](const int& v) { return v % 10 == 0; };
using DivisibleBy10 = A<int, fn_validator<events::number_parsing_finished, div_by_10>>;

constexpr bool test_fn_validator_number() {
    DivisibleBy10 val{};

    // Should pass: 100 is divisible by 10
    if (!Parse(val, std::string_view("100"))) return false;
    if (val.value != 100) return false;

    // Should pass: 0 is divisible by 10
    if (!Parse(val, std::string_view("0"))) return false;
    if (val.value != 0) return false;

    // Should fail: 15 is not divisible by 10
    if (Parse(val, std::string_view("15"))) return false;

    // Should fail: 7 is not divisible by 10
    if (Parse(val, std::string_view("7"))) return false;

    return true;
}
static_assert(test_fn_validator_number(),
              "fn_validator should validate number divisibility");

// ============================================================================
// Test: fn_validator on number_parsing_finished (even check)
// ============================================================================

constexpr auto is_even = [](const int& v) { return v % 2 == 0; };
using EvenNumber = A<int, fn_validator<events::number_parsing_finished, is_even>>;

constexpr bool test_fn_validator_even() {
    EvenNumber val{};

    // Should pass: even numbers
    if (!Parse(val, std::string_view("2"))) return false;
    if (!Parse(val, std::string_view("4"))) return false;
    if (!Parse(val, std::string_view("100"))) return false;
    if (!Parse(val, std::string_view("0"))) return false;
    if (!Parse(val, std::string_view("-2"))) return false;

    // Should fail: odd numbers
    if (Parse(val, std::string_view("3"))) return false;
    if (Parse(val, std::string_view("1"))) return false;
    if (Parse(val, std::string_view("-1"))) return false;

    return true;
}
static_assert(test_fn_validator_even(),
              "fn_validator should validate even numbers");

// ============================================================================
// Test: fn_validator on bool_parsing_finished
// ============================================================================

// Only accept true values
constexpr auto must_be_true = [](const bool& v) { return v == true; };
using MustBeTrue = A<bool, fn_validator<events::bool_parsing_finished, must_be_true>>;

constexpr bool test_fn_validator_bool() {
    MustBeTrue val{};

    // Should pass: true
    if (!Parse(val, std::string_view("true"))) return false;
    if (val.value != true) return false;

    // Should fail: false
    if (Parse(val, std::string_view("false"))) return false;

    return true;
}
static_assert(test_fn_validator_bool(),
              "fn_validator should validate bool values");

// ============================================================================
// Test: fn_validator on array_parsing_finished (non-empty check)
// ============================================================================

constexpr auto non_empty_check = [](const auto&, std::size_t count) { return count != 0; };
using NonEmptyArray = A<std::array<int, 10>, fn_validator<events::array_parsing_finished, non_empty_check>>;

constexpr bool test_fn_validator_array() {
    NonEmptyArray val{};

    // Should pass: non-empty array
    if (!Parse(val, std::string_view("[1,2,3]"))) return false;

    // Should fail: empty array
    if (Parse(val, std::string_view("[]"))) return false;

    return true;
}
static_assert(test_fn_validator_array(),
              "fn_validator should validate non-empty arrays");

// ============================================================================
// Test: fn_validator on struct field
// ============================================================================

// Divisibility by 5 check
constexpr auto div_by_5 = [](const int& v) { return v % 5 == 0; };
using DivisibleBy5 = A<int, fn_validator<events::number_parsing_finished, div_by_5>>;

struct Config {
    DivisibleBy5 port;  // Port must be divisible by 5
    int timeout;
};

constexpr bool test_fn_validator_in_struct() {
    Config cfg{};

    // Should pass: port 8080 % 5 == 0
    if (!Parse(cfg, std::string_view(R"({"port": 8080, "timeout": 30})"))) return false;
    if (cfg.port.value != 8080) return false;
    if (cfg.timeout != 30) return false;

    // Should fail: port 8081 is not divisible by 5
    if (Parse(cfg, std::string_view(R"({"port": 8081, "timeout": 30})"))) return false;

    return true;
}
static_assert(test_fn_validator_in_struct(),
              "fn_validator should work in struct fields");

// ============================================================================
// Test: Multiple fn_validators combined
// ============================================================================

constexpr auto div_by_3 = [](const int& v) { return v % 3 == 0; };

using BoundedDivisibleBy3 = A<int,
    range<0, 100>,  // Built-in range validator
    fn_validator<events::number_parsing_finished, div_by_3>
>;

constexpr bool test_fn_validator_combined() {
    BoundedDivisibleBy3 val{};

    // Should pass: 0, 30, 99 are in range [0,100] and divisible by 3
    if (!Parse(val, std::string_view("0"))) return false;
    if (!Parse(val, std::string_view("30"))) return false;
    if (!Parse(val, std::string_view("99"))) return false;

    // Should fail: out of range
    if (Parse(val, std::string_view("102"))) return false;
    if (Parse(val, std::string_view("-3"))) return false;

    // Should fail: not divisible by 3
    if (Parse(val, std::string_view("50"))) return false;

    return true;
}
static_assert(test_fn_validator_combined(),
              "fn_validator should work with other validators");

