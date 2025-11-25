#pragma once

#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <string_view>
#include <cstddef>
#include <pfr.hpp>

namespace TestHelpers {

// ============================================================================
// Parse Helpers
// ============================================================================

/// Check that parsing succeeds
template<typename T, typename Container>
constexpr bool ParseSucceeds(T& obj, const Container& json) {
    return static_cast<bool>(JsonFusion::Parse(obj, json));
}

/// Check that parsing succeeds with string_view
template<typename T>
constexpr bool ParseSucceeds(T& obj, std::string_view json) {
    return static_cast<bool>(JsonFusion::Parse(obj, json));
}

/// Check that parsing fails (any error)
template<typename T, typename Container>
constexpr bool ParseFails(T& obj, const Container& json) {
    return !JsonFusion::Parse(obj, json);
}

/// Check that parsing fails with specific error code
template<typename T, typename Container>
constexpr bool ParseFailsWith(T& obj, const Container& json, JsonFusion::ParseError expected_error) {
    auto result = JsonFusion::Parse(obj, json);
    return !result && result.error() == expected_error;
}

/// Check that parsing fails with specific error code (string_view)
template<typename T>
constexpr bool ParseFailsWith(T& obj, std::string_view json, JsonFusion::ParseError expected_error) {
    auto result = JsonFusion::Parse(obj, json);
    return !result && result.error() == expected_error;
}

/// Check that parsing fails with specific error code at approximate position
/// Allows tolerance of +/- 2 characters for position
template<typename T>
constexpr bool ParseFailsAt(T& obj, std::string_view json, 
                            JsonFusion::ParseError expected_error, 
                            std::size_t expected_pos_approx,
                            std::size_t tolerance = 2) {
    auto result = JsonFusion::Parse(obj, json);
    if (result) return false;  // Expected failure
    if (result.error() != expected_error) return false;  // Wrong error code
    
    std::size_t actual_pos = result.pos() - json.data();
    return actual_pos >= (expected_pos_approx - tolerance) 
        && actual_pos <= (expected_pos_approx + tolerance);
}

// ============================================================================
// Serialize Helpers
// ============================================================================

/// Check that serialization succeeds
template<typename T, typename OutIter>
constexpr bool SerializeSucceeds(const T& obj, OutIter& out, OutIter end) {
    return JsonFusion::Serialize(obj, out, end);
}

/// Check that serialization fails
template<typename T, typename OutIter>
constexpr bool SerializeFails(const T& obj, OutIter& out, OutIter end) {
    return !JsonFusion::Serialize(obj, out, end);
}

// ============================================================================
// Round-Trip Helpers
// ============================================================================

/// Parse JSON, serialize back, compare byte-by-byte
template<typename T, std::size_t BufferSize = 1024>
constexpr bool RoundTripEquals(T& obj, std::string_view original_json) {
    // Parse
    if (!JsonFusion::Parse(obj, original_json)) {
        return false;
    }
    
    // Serialize
    std::array<char, BufferSize> buf{};
    char* out = buf.data();
    char* end = out + buf.size();
    
    if (!JsonFusion::Serialize(obj, out, end)) {
        return false;
    }
    
    // Compare
    std::string_view serialized(buf.data(), out - buf.data());
    return serialized == original_json;
}

/// Parse JSON, serialize back, parse again, verify fields unchanged
template<typename T, std::size_t BufferSize = 1024>
constexpr bool RoundTripPreservesFields(T& obj1, T& obj2, std::string_view json) {
    // First parse
    if (!JsonFusion::Parse(obj1, json)) {
        return false;
    }
    
    // Serialize
    std::array<char, BufferSize> buf{};
    char* out = buf.data();
    char* end = out + buf.size();
    
    if (!JsonFusion::Serialize(obj1, out, end)) {
        return false;
    }
    
    // Second parse
    std::string_view serialized(buf.data(), out - buf.data());
    if (!JsonFusion::Parse(obj2, serialized)) {
        return false;
    }
    
    // Fields must match (caller compares specific fields)
    return true;
}

// ============================================================================
// String Comparison Helpers
// ============================================================================

/// Compare two char arrays for equality up to null terminator
template<std::size_t N1, std::size_t N2>
constexpr bool CStrEqual(const std::array<char, N1>& a, const std::array<char, N2>& b) {
    std::size_t i = 0;
    while (i < N1 && i < N2) {
        if (a[i] != b[i]) return false;
        if (a[i] == '\0') return true;  // Both reached null terminator
        ++i;
    }
    // If we're here, one array ended without null terminator match
    return false;
}

/// Compare char array with string literal
template<std::size_t N>
constexpr bool CStrEqual(const std::array<char, N>& arr, const char* str) {
    std::size_t i = 0;
    while (i < N && str[i] != '\0') {
        if (arr[i] != str[i]) return false;
        ++i;
    }
    return arr[i] == '\0' && str[i] == '\0';
}

/// Check if char array starts with given string
template<std::size_t N>
constexpr bool CStrStartsWith(const std::array<char, N>& arr, const char* prefix) {
    std::size_t i = 0;
    while (prefix[i] != '\0') {
        if (i >= N || arr[i] != prefix[i]) return false;
        ++i;
    }
    return true;
}

// ============================================================================
// Array Comparison Helpers
// ============================================================================

/// Compare two std::arrays element by element
template<typename T, std::size_t N>
constexpr bool ArrayEqual(const std::array<T, N>& a, const std::array<T, N>& b) {
    for (std::size_t i = 0; i < N; ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Compare array with initializer list
template<typename T, std::size_t N>
constexpr bool ArrayEqual(const std::array<T, N>& arr, const T (&expected)[N]) {
    for (std::size_t i = 0; i < N; ++i) {
        if (arr[i] != expected[i]) return false;
    }
    return true;
}

// ============================================================================
// Struct Comparison Helpers (Using PFR)
// ============================================================================

/// Compare two values field-by-field (constexpr-safe)
/// Handles nested structs, arrays, optionals
template<typename T>
constexpr bool DeepEqual(const T& a, const T& b) {
    // For primitive types, use operator==
    if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
        return a == b;
    }
    // For optionals
    else if constexpr (requires { a.has_value(); a.value(); }) {
        if (a.has_value() != b.has_value()) return false;
        if (!a.has_value()) return true;  // Both are nullopt
        return DeepEqual(a.value(), b.value());
    }
    // For aggregate types (structs), use PFR to compare field-by-field
    else if constexpr (pfr::is_implicitly_reflectable_v<T, T>) {
        constexpr std::size_t fields_count = pfr::tuple_size_v<T>;
        return [&]<std::size_t... I>(std::index_sequence<I...>) {
            return (... && DeepEqual(pfr::get<I>(a), pfr::get<I>(b)));
        }(std::make_index_sequence<fields_count>{});
    }
    // For char arrays, only compare up to null terminator
    else if constexpr (requires { a.data(); a.size(); } && 
                      std::is_same_v<std::remove_cvref_t<decltype(*a.data())>, char>) {
        std::size_t i = 0;
        while (i < a.size()) {
            if (a[i] != b[i]) return false;
            if (a[i] == '\0') return true;  // Both reached null
            ++i;
        }
        return true;  // Both arrays fully matched
    }
    // For arrays/containers with elements
    else if constexpr (requires { a.begin(); a.end(); a.size(); }) {
        if (a.size() != b.size()) return false;
        auto it_a = a.begin();
        auto it_b = b.begin();
        while (it_a != a.end()) {
            if (!DeepEqual(*it_a, *it_b)) return false;
            ++it_a;
            ++it_b;
        }
        return true;
    }
    // Fallback to operator==
    else {
        return a == b;
    }
}

/// Compare two structs field-by-field
template<typename T>
constexpr bool StructEqual(const T& a, const T& b) {
    return DeepEqual(a, b);
}

/// Parse JSON and compare result with expected struct
/// Returns true if parsing succeeds AND all fields match
template<typename T>
constexpr bool ParseAndCompare(T& obj, std::string_view json, const T& expected) {
    if (!JsonFusion::Parse(obj, json)) {
        return false;
    }
    return DeepEqual(obj, expected);
}

/// Parse JSON and verify using custom comparison lambda
/// Useful when you only want to check specific fields
template<typename T, typename Comparator>
constexpr bool ParseAndVerify(T& obj, std::string_view json, Comparator&& cmp) {
    if (!JsonFusion::Parse(obj, json)) {
        return false;
    }
    return cmp(obj);
}

// ============================================================================
// Ultra-Minimal Test Helpers (Eliminate Boilerplate)
// ============================================================================

/// Short alias for std::string_view to reduce verbosity
/// Usage: json(R"({"x": 42})")
constexpr auto json = [](const char* s) constexpr { return std::string_view(s); };

/// One-line parse test: TestParse<Type>(json, expected)
/// Eliminates lambda boilerplate and object declaration
template<typename T>
constexpr bool TestParse(const char* json_str, const T& expected) {
    T obj{};
    return ParseAndCompare(obj, std::string_view(json_str), expected);
}

/// One-line parse test with custom verification
template<typename T, typename Verifier>
constexpr bool TestParse(const char* json_str, Verifier&& verify) {
    T obj{};
    return ParseAndVerify(obj, std::string_view(json_str), std::forward<Verifier>(verify));
}

/// One-line error test: TestParseError<Type>(json, error_code)
template<typename T>
constexpr bool TestParseError(const char* json_str, JsonFusion::ParseError expected_error) {
    T obj{};
    return ParseFailsWith(obj, std::string_view(json_str), expected_error);
}

/// One-line serialize test: TestSerialize(obj, expected_json)
template<typename T, std::size_t BufferSize = 512>
constexpr bool TestSerialize(const T& obj, const char* expected_json) {
    std::array<char, BufferSize> buf{};
    char* out = buf.data();
    char* end = out + buf.size();
    
    if (!JsonFusion::Serialize(obj, out, end)) {
        return false;
    }
    
    std::string_view result(buf.data(), out - buf.data());
    std::string_view expected(expected_json);
    return result == expected;
}

/// One-line round-trip test
template<typename T>
constexpr bool TestRoundTrip(const char* json_str, const T& expected) {
    T obj{};
    return RoundTripEquals(obj, std::string_view(json_str));
}

} // namespace TestHelpers

