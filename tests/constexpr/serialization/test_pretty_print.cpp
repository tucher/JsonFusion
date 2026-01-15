#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/json.hpp>
#include <string>
#include <array>

using namespace JsonFusion;

// ============================================================================
// Test: Pretty print adds newlines
// ============================================================================

constexpr bool test_pretty_print_has_newlines() {
    struct Simple {
        int x;
        int y;
    };

    Simple s{10, 20};

    std::array<char, 256> buffer{};
    JsonIteratorWriter<char*, char*, true> writer(buffer.data(), buffer.data() + buffer.size());

    auto result = SerializeWithWriter(s, writer);
    if (!result) return false;

    std::string_view output(buffer.data(), result.bytesWritten());

    // Pretty print should contain newlines
    bool has_newline = false;
    for (char c : output) {
        if (c == '\n') {
            has_newline = true;
            break;
        }
    }

    return has_newline;
}
static_assert(test_pretty_print_has_newlines(),
              "Pretty print output should contain newlines");

// ============================================================================
// Test: Pretty print adds indentation
// ============================================================================

constexpr bool test_pretty_print_has_indentation() {
    struct Simple {
        int value;
    };

    Simple s{42};

    std::array<char, 256> buffer{};
    JsonIteratorWriter<char*, char*, true> writer(buffer.data(), buffer.data() + buffer.size());

    auto result = SerializeWithWriter(s, writer);
    if (!result) return false;

    std::string_view output(buffer.data(), result.bytesWritten());

    // Should have spaces for indentation
    bool has_leading_spaces = false;
    bool after_newline = false;
    for (char c : output) {
        if (c == '\n') {
            after_newline = true;
        } else if (after_newline && c == ' ') {
            has_leading_spaces = true;
            break;
        } else {
            after_newline = false;
        }
    }

    return has_leading_spaces;
}
static_assert(test_pretty_print_has_indentation(),
              "Pretty print should add indentation after newlines");

// ============================================================================
// Test: Compact (non-pretty) has no newlines
// ============================================================================

constexpr bool test_compact_no_newlines() {
    struct Simple {
        int x;
        int y;
    };

    Simple s{10, 20};

    std::string output;
    auto result = Serialize(s, output);
    if (!result) return false;

    // Compact should NOT contain newlines
    for (char c : output) {
        if (c == '\n') return false;
    }

    return true;
}
static_assert(test_compact_no_newlines(),
              "Compact output should not contain newlines");

// ============================================================================
// Test: Pretty print nested structure
// ============================================================================

constexpr bool test_pretty_print_nested() {
    struct Inner {
        int a;
    };
    struct Outer {
        Inner inner;
        int b;
    };

    Outer o{{100}, 200};

    std::array<char, 512> buffer{};
    JsonIteratorWriter<char*, char*, true> writer(buffer.data(), buffer.data() + buffer.size());

    auto result = SerializeWithWriter(o, writer);
    if (!result) return false;

    std::string_view output(buffer.data(), result.bytesWritten());

    // Should have multiple newlines for nested structure
    int newline_count = 0;
    for (char c : output) {
        if (c == '\n') newline_count++;
    }

    // At minimum: after {, after inner:{, after inner.a, after }, after b, before }
    return newline_count >= 4;
}
static_assert(test_pretty_print_nested(),
              "Pretty print should handle nested structures");

// ============================================================================
// Test: Pretty print array
// ============================================================================

constexpr bool test_pretty_print_array() {
    struct WithArray {
        std::array<int, 3> values;
    };

    WithArray w{{1, 2, 3}};

    std::array<char, 512> buffer{};
    JsonIteratorWriter<char*, char*, true> writer(buffer.data(), buffer.data() + buffer.size());

    auto result = SerializeWithWriter(w, writer);
    if (!result) return false;

    std::string_view output(buffer.data(), result.bytesWritten());

    // Array elements should be on separate lines
    int newline_count = 0;
    for (char c : output) {
        if (c == '\n') newline_count++;
    }

    return newline_count >= 3;
}
static_assert(test_pretty_print_array(),
              "Pretty print should format arrays nicely");

// ============================================================================
// Test: Pretty print has space after colon
// ============================================================================

constexpr bool test_pretty_print_space_after_colon() {
    struct Simple {
        int value;
    };

    Simple s{42};

    std::array<char, 256> buffer{};
    JsonIteratorWriter<char*, char*, true> writer(buffer.data(), buffer.data() + buffer.size());

    auto result = SerializeWithWriter(s, writer);
    if (!result) return false;

    std::string_view output(buffer.data(), result.bytesWritten());

    // Find ": " pattern (colon followed by space)
    for (std::size_t i = 0; i + 1 < output.size(); ++i) {
        if (output[i] == ':' && output[i + 1] == ' ') {
            return true;
        }
    }

    return false;
}
static_assert(test_pretty_print_space_after_colon(),
              "Pretty print should have space after colon");

// ============================================================================
// Test: Pretty printed JSON can be parsed back
// ============================================================================

constexpr bool test_pretty_print_roundtrip() {
    struct Config {
        int port;
        bool enabled;
        std::array<char, 16> name{};
    };

    Config original{8080, true, {}};
    original.name[0] = 't'; original.name[1] = 'e'; original.name[2] = 's';
    original.name[3] = 't'; original.name[4] = '\0';

    // Serialize with pretty print
    std::array<char, 512> buffer{};
    JsonIteratorWriter<char*, char*, true> writer(buffer.data(), buffer.data() + buffer.size());
    auto ser_result = SerializeWithWriter(original, writer);
    if (!ser_result) return false;

    std::string_view json(buffer.data(), ser_result.bytesWritten());

    // Parse back
    Config parsed;
    auto parse_result = Parse(parsed, json);
    if (!parse_result) return false;

    // Verify values match
    if (parsed.port != original.port) return false;
    if (parsed.enabled != original.enabled) return false;
    if (!TestHelpers::CStrEqual(parsed.name, original.name)) return false;

    return true;
}
static_assert(test_pretty_print_roundtrip(),
              "Pretty printed JSON should be parseable");

