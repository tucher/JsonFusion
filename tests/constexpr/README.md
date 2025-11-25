# JsonFusion Constexpr Tests

Compile-time tests for JsonFusion using C++23 `static_assert`. Tests run during compilation—if it compiles, tests passed.

## Quick Start

```bash
# Run all tests
./run_tests.sh

# Run single test
g++ -std=c++23 -I../../include -c test_parse_int.cpp
```

## Writing Tests - Minimal Syntax

```cpp
#include "test_helpers.hpp"
using namespace TestHelpers;

struct Config { int port; bool enabled; };

// Parse test (1 line!)
static_assert(TestParse(R"({"port": 8080, "enabled": true})", Config{8080, true}));

// Error test (1 line!)
static_assert(TestParseError<Config>(R"({"port": "bad"})", ParseError::ILLFORMED_NUMBER));

// Serialize test (1 line!)
static_assert(TestSerialize(Config{8080, true}, R"({"port":8080,"enabled":true})"));

// Custom verification
static_assert(TestParse<Config>(R"({"port": 8080, "enabled": true})", 
    [](const Config& c) { return c.port == 8080; }));
```

## Available Test Helpers

### Parsing
```cpp
TestParse(json, expected)              // Parse + compare struct
TestParse<T>(json, lambda)             // Parse + custom verify
TestParseError<T>(json, error_code)    // Parse + check error
```

### Serialization
```cpp
TestSerialize(obj, expected_json)      // Serialize + compare
TestRoundTrip(json, expected)          // Parse → Serialize → Parse
```

### Advanced (when needed)
```cpp
ParseSucceeds(obj, json)               // Just check parsing works
ParseFails(obj, json)                  // Just check parsing fails
ParseFailsWith(obj, json, error)       // Check specific error
ParseFailsAt(obj, json, error, pos)    // Check error at position

StructEqual(a, b)                      // Deep compare (handles nested/optionals/arrays)
ParseAndCompare(obj, json, expected)   // Parse + StructEqual
ParseAndVerify(obj, json, lambda)      // Parse + custom lambda

SerializeSucceeds(obj, out, end)       // Just check serialization works
```

## How It Works

### 1. Struct Comparison - Automatic & Deep

`StructEqual()` and `TestParse()` use PFR-powered deep comparison:

```cpp
struct Nested {
    int id;
    struct Point { int x, y; } pos;
    std::optional<int> value;
    std::array<int, 3> data;
};

// All fields compared recursively - one line!
static_assert(TestParse(R"(...)", Nested{42, {10, 20}, 100, {1,2,3}}));
```

**Handles automatically:**
- ✅ Nested structs (any depth)
- ✅ Arrays (`std::array`)
- ✅ Optionals (`std::optional`)  
- ✅ Char arrays (null-terminator aware)
- ✅ Mixed combinations

### 2. Error Testing

```cpp
// Check specific error code
static_assert(TestParseError<Config>(
    R"({"port": "string"})",
    ParseError::ILLFORMED_NUMBER));

// Check error at approximate position
static_assert([]() constexpr {
    Config c;
    return ParseFailsAt(c, std::string_view(R"({"port": "bad"})"),
        ParseError::ILLFORMED_NUMBER, 10, /*tolerance=*/2);
}());
```

**Common Error Codes:**
- `ILLFORMED_NUMBER` - Invalid number or type mismatch
- `ILLFORMED_BOOL` - Invalid boolean
- `ILLFORMED_STRING` - Invalid string/escaping
- `UNEXPECTED_END_OF_DATA` - Unclosed braces/brackets
- `UNEXPECTED_SYMBOL` - Invalid character
- `FIXED_SIZE_CONTAINER_OVERFLOW` - String/array too large
- `NUMBER_OUT_OF_RANGE_SCHEMA_ERROR` - Validation constraint failed

### 3. Type Inference

The type is automatically inferred from the expected value:

```cpp
static_assert(TestParse(R"({"port": 8080})", Config{8080}));
//                                           ^^^^^^ Type inferred here
```

No need to declare objects or specify types explicitly.

## Test Organization

```
tests/constexpr/
├── test_helpers.hpp          # All helper functions
├── run_tests.sh              # Test runner (searches all subdirectories)
├── README.md                 # This file
├── TEST_PLAN.md              # Comprehensive test roadmap (150+ tests)
├── CHECKLIST.md              # Progress tracker
│
├── primitives/               # Primitive type tests (int, bool, string)
│   ├── test_parse_int.cpp
│   ├── test_parse_bool.cpp
│   └── test_parse_char_array.cpp
│
├── serialization/            # Serialization & round-trip tests
│   ├── test_serialize_int.cpp
│   └── test_serialize_bool.cpp
│
├── errors/                   # Error handling tests
│   └── test_error_demo.cpp
│
├── composite/                # Nested structs, arrays, optionals
├── validation/               # Validation constraints (range, length, items)
├── annotations/              # Annotated<> options (key, as_array, not_json)
├── streaming/                # Streaming producers & consumers
├── json_spec/                # JSON RFC 8259 compliance
└── integration/              # Real-world scenarios
```

## Coverage Status

See `CHECKLIST.md` for detailed progress tracking.

**Current:** 6 test files, 21 tests passing

**Planned:** 150+ tests covering:
- All primitive types (integers, bool, strings)
- Composite types (nested structs, arrays, optionals)
- JSON spec compliance (RFC 8259)
- Validation constraints (range, length, items)
- Annotated<> options (key, as_array, not_json, etc.)
- Error handling (all error codes)
- Edge cases (zero-sized, alignment, limits)

## Why Constexpr Tests?

1. **Zero runtime overhead** - Tests run at compile time only
2. **Proof of purity** - If it works in constexpr, no hidden allocations/globals
3. **Fast CI** - Just compile, no test binary execution
4. **Type-safe** - Compiler catches all errors
5. **Self-documenting** - Shows exactly what works at compile time

## Examples

### Primitive Types
```cpp
static_assert(TestParse(R"({"x": 42})", Struct{42}));
static_assert(TestParse(R"({"x": -123})", Struct{-123}));
static_assert(TestParse(R"({"x": true})", Struct{true}));
```

### Nested Structs
```cpp
struct Outer {
    int id;
    struct Inner { int x, y; } pos;
};

static_assert(TestParse(R"({"id": 1, "pos": {"x": 10, "y": 20}})", 
    Outer{1, {10, 20}}));
```

### Arrays
```cpp
struct WithArray {
    std::array<int, 3> values;
};

static_assert(TestParse(R"({"values": [1, 2, 3]})", 
    WithArray{{1, 2, 3}}));
```

### Optionals
```cpp
struct WithOptional {
    std::optional<int> value;
};

static_assert(TestParse(R"({"value": 42})", WithOptional{42}));
static_assert(TestParse(R"({"value": null})", WithOptional{std::nullopt}));
```

### Error Cases
```cpp
static_assert(TestParseError<Config>(
    R"({"port": "not_a_number"})", 
    ParseError::ILLFORMED_NUMBER));

static_assert(TestParseError<Config>(
    R"({"port": 80)", 
    ParseError::UNEXPECTED_END_OF_DATA));
```

### Round-Trips
```cpp
static_assert(TestRoundTrip(R"({"port":8080,"enabled":true})", 
    Config{8080, true}));
```

## Notes

- **C++23 required** - Takes advantage of improved constexpr support
- **Only fixed-size containers** - `std::array` works, `std::vector` doesn't (dynamic allocation)
- **Floats tested separately** - Not constexpr-compatible with current implementation
- **Null-termination guaranteed** - Char arrays are always null-terminated by parser

## See Also

- `TEST_PLAN.md` - Comprehensive test coverage plan
- `CHECKLIST.md` - Implementation progress
- `test_error_demo.cpp` - Examples of error testing patterns
