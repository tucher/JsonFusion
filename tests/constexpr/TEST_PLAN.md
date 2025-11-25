# JsonFusion Constexpr Test Coverage Plan

This document outlines comprehensive test coverage for JsonFusion's compile-time guarantees. Since constexpr tests are fast to write and provide strong guarantees, we aim for exhaustive coverage.

## Organization Principles

- **One test per file** - Fast iteration, clear failures, parallel compilation
- **Multiple `static_assert`s per file** - Related test cases grouped together
- **Naming convention**: `test_<category>_<feature>.cpp`
- **Each test is self-contained** - Minimal dependencies, clear purpose

---

## 1. Primitive Types

### 1.1 Integer Types (`test_parse_integers_*.cpp`)

**Status**: ✅ `test_parse_int.cpp` (basic coverage)

**Needed**:
- `test_parse_integers_signed.cpp`
  - `int8_t`, `int16_t`, `int32_t`, `int64_t`
  - Positive, negative, zero for each
  - Min/max values for each type
  - `-1`, `-128`, `-32768`, etc.

- `test_parse_integers_unsigned.cpp`
  - `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`
  - Zero, max values
  - `255`, `65535`, etc.

- `test_parse_integers_edge_cases.cpp`
  - Leading zeros: `{"x": 007}` (should work or reject per JSON spec?)
  - Negative zero: `{"x": -0}`
  - Large numbers that fit/don't fit
  - Overflow detection (if implemented)
  - Whitespace around numbers: `{"x":  42  }`

### 1.2 Boolean (`test_parse_bool.cpp`)

**Status**: ✅ Basic coverage

**Needed**:
- `test_parse_bool_edge_cases.cpp`
  - Whitespace around values: `{"flag":  true  }`
  - Mixed case (should reject): `{"flag": True}`, `{"flag": TRUE}`
  - Boolean in different contexts (array element, nested object)

### 1.3 Char Arrays (`test_parse_char_array.cpp`)

**Status**: ✅ Null-termination verified

**Needed**:
- `test_parse_strings_escaping.cpp`
  - Escape sequences: `\"`, `\\`, `\/`, `\b`, `\f`, `\n`, `\r`, `\t`
  - Unicode escapes: `\u0041` (A), `\u00E9` (é)
  - All standard JSON escape sequences from RFC 8259

- `test_parse_strings_edge_cases.cpp`
  - Empty string: `""`
  - Single character
  - String exactly filling buffer (no room for null terminator)
  - String too long for buffer (truncation/error)
  - Consecutive strings in array
  - Strings with only whitespace: `"   "`

- `test_parse_strings_special_chars.cpp`
  - Unicode characters (if supported)
  - ASCII control characters (should be escaped in JSON)
  - High-bit ASCII (0x80-0xFF)

---

## 2. Composite Types

### 2.1 Nested Structs

- `test_parse_nested_flat.cpp`
  - One level of nesting
  - Two levels of nesting
  - Three+ levels of nesting

- `test_parse_nested_complex.cpp`
  - Nested structs with arrays
  - Arrays of nested structs
  - Mixed primitive + nested fields

- `test_parse_nested_empty.cpp`
  - Empty nested struct: `struct Inner {};`
  - Struct with only nested empty structs

### 2.2 Arrays - Fixed Size (`std::array<T, N>`)

- `test_parse_array_primitives.cpp`
  - `std::array<int, N>` for various N
  - `std::array<bool, N>`
  - `std::array<char, N>` (different from string!)
  - Empty array: `std::array<int, 0>`
  - Single element: `std::array<int, 1>`

- `test_parse_array_nested.cpp`
  - `std::array<NestedStruct, N>`
  - 2D arrays: `std::array<std::array<int, 3>, 3>`
  - 3D arrays (if practical)

- `test_parse_array_edge_cases.cpp`
  - Array with trailing comma (should reject per JSON spec)
  - Whitespace around elements: `[  1  ,  2  ]`
  - Mixed types (should fail type checking)
  - Array too short (missing elements)
  - Array too long (excess elements - error handling)

### 2.3 Optionals (`std::optional<T>`)

- `test_parse_optional_primitives.cpp`
  - `std::optional<int>` - null vs present
  - `std::optional<bool>` - null vs true/false
  - `std::optional<std::array<char, N>>` - null vs string

- `test_parse_optional_nested.cpp`
  - `std::optional<NestedStruct>` - null vs object
  - Deeply nested optionals: `std::optional<std::optional<int>>`
  - Optional arrays: `std::optional<std::array<int, N>>`

- `test_parse_optional_edge_cases.cpp`
  - Field present but null
  - Field absent entirely (if supported)
  - `null` vs `"null"` (string)
  - Optional in various positions (first, middle, last field)

---

## 3. JSON Standard Compliance

### 3.1 Whitespace Handling

- `test_json_whitespace.cpp`
  - Leading/trailing whitespace in document
  - Whitespace between keys and colons: `{"key" : "value"}`
  - Whitespace around commas
  - Tabs, newlines, carriage returns
  - Multiple consecutive spaces
  - No whitespace (compact JSON)

### 3.2 Field Ordering

- `test_json_field_order.cpp`
  - Fields in different orders than struct definition
  - All permutations for 3-field struct
  - Fields at different nesting levels

### 3.3 JSON Syntax Edge Cases

- `test_json_syntax_valid.cpp`
  - Minimal valid JSON: `{}`
  - Deeply nested objects (10+ levels)
  - Long field names
  - Many fields (20+)
  - Large arrays (100+ elements)

- `test_json_syntax_invalid.cpp` (error detection)
  - Missing closing brace: `{"x": 1`
  - Missing closing bracket: `[1, 2`
  - Missing comma: `{"x": 1 "y": 2}`
  - Extra comma: `{"x": 1,}`
  - Missing colon: `{"x" 1}`
  - Double colon: `{"x":: 1}`
  - Unquoted keys: `{x: 1}`
  - Single quotes: `{'x': 1}`
  - Trailing text after JSON: `{"x": 1} garbage`

---

## 4. Validation Constraints

### 4.1 Range Validation

- `test_validation_range_int.cpp`
  - `Annotated<int, range<0, 100>>` - valid values at boundaries
  - Below min, above max (error detection)
  - Exactly at min, exactly at max
  - Negative ranges: `range<-100, -10>`
  - Single-value range: `range<42, 42>`

- `test_validation_range_float.cpp` (if floats become constexpr-compatible)
  - Fractional boundaries
  - Very small ranges

### 4.2 Length Validation

- `test_validation_string_length.cpp`
  - `min_length<N>`, `max_length<N>`
  - Empty string with `min_length<1>` (error)
  - Exactly at boundaries
  - Both constraints together: `min_length<5>, max_length<10>`

### 4.3 Array Size Validation

- `test_validation_array_items.cpp`
  - `min_items<N>`, `max_items<N>`
  - Empty array constraints
  - Exactly at boundaries
  - Both constraints together

---

## 5. Annotated<> Options

### 5.1 Key Remapping

- `test_annotated_key.cpp`
  - `Annotated<int, key<"json_name">>` with different C++ name
  - Keys with special characters: `"field-name"`, `"field.name"`
  - Keys with underscores, numbers
  - Empty key: `""`
  - Very long keys

### 5.2 as_array

- `test_annotated_as_array.cpp`
  - Struct serialized as JSON array
  - Nested structs with `as_array`
  - Mixed: some fields as_array, some not
  - Order dependency: fields must match array order

### 5.3 not_json

- `test_annotated_not_json.cpp`
  - Field marked `not_json` is skipped in parsing
  - Field marked `not_json` is skipped in serialization
  - Derived/computed fields
  - Internal state fields

### 5.4 not_required

- `test_annotated_not_required.cpp`
  - Field can be absent from JSON
  - Default value is used
  - Present vs absent behavior
  - Difference from `std::optional`

### 5.5 allow_excess_fields

- `test_annotated_allow_excess.cpp`
  - Object with extra JSON fields that aren't in struct
  - Per-object policy vs global
  - Nested objects with different policies

---

## 6. Serialization

### 6.1 Primitives

**Status**: ✅ `test_serialize_int.cpp`, `test_serialize_bool.cpp`

**Needed**:
- `test_serialize_integers_all_types.cpp`
  - All int types serialize correctly
  - Negative values
  - Zero
  - Min/max values

- `test_serialize_strings.cpp`
  - Char arrays serialize with proper escaping
  - Null terminator is not serialized
  - Empty strings: `""`
  - Special characters are escaped

### 6.2 Round-Trip Tests

- `test_roundtrip_primitives.cpp`
  - Parse → Serialize → Parse again
  - Verify byte-for-byte equality (or semantic equality)
  - All primitive types

- `test_roundtrip_nested.cpp`
  - Complex nested structures
  - Arrays of structs
  - Optional fields

- `test_roundtrip_annotated.cpp`
  - Structures with `key<>` remapping
  - Structures with `as_array`
  - Mixed annotations

### 6.3 Formatting

- `test_serialize_formatting.cpp`
  - No extra whitespace (compact)
  - Field order matches struct definition (or JSON order for parsing)
  - Array element ordering
  - Null serialization: `null` not `NULL` or `nullptr`
  - Boolean serialization: lowercase `true`/`false`

---

## 7. Streaming (Producers & Consumers)

### 7.1 Consuming Streamers

- `test_streamer_consumer_primitives.cpp`
  - Consumer for `int`, `bool`, `char`
  - `reset()`, `consume()`, `finalize()` lifecycle
  - Counting elements
  - Element validation in `consume()`

- `test_streamer_consumer_structs.cpp`
  - Consumer for nested structs
  - Consumer with `value_type` = annotated struct
  - Multiple consumers in same top-level object

- `test_streamer_consumer_edge_cases.cpp`
  - Empty array: only `reset()` and `finalize()` called
  - Single element
  - Early termination (returning false from `consume()`)

### 7.2 Producing Streamers

- `test_streamer_producer_primitives.cpp`
  - Producer generating `int`, `bool`, etc.
  - `reset()`, `read()` lifecycle
  - Returning `stream_read_result::end`
  - Returning `stream_read_result::error`

- `test_streamer_producer_structs.cpp`
  - Producer generating structs
  - Producer with `value_type` = annotated struct

- `test_streamer_producer_nested.cpp`
  - Producer generating other producers (2D arrays)
  - 3D arrays via nested producers
  - Jagged arrays

### 7.3 Streamer Concepts

- `test_streamer_concepts.cpp`
  - `ConsumingStreamerLike` concept verification
  - `ProducingStreamerLike` concept verification
  - Compilation errors for non-conforming types (negative tests)

---

## 8. Error Handling

### 8.1 Type Mismatches

- `test_error_type_mismatch.cpp`
  - String where int expected
  - Int where bool expected
  - Array where object expected
  - Object where array expected
  - Null where non-optional expected

### 8.2 Malformed JSON

- `test_error_malformed_json.cpp`
  - Unclosed braces, brackets
  - Missing quotes on strings
  - Invalid escape sequences
  - Truncated JSON
  - Invalid UTF-8 (if applicable)

### 8.3 Validation Failures

- `test_error_validation_range.cpp`
  - Value outside `range<>`
  - Error position reported correctly

- `test_error_validation_length.cpp`
  - String too short/long
  - Array too short/long

### 8.4 Buffer Overflow

- `test_error_string_overflow.cpp`
  - String longer than `std::array<char, N>`
  - Behavior: truncate, error, or undefined?

- `test_error_array_overflow.cpp`
  - JSON array longer than `std::array<T, N>`

### 8.5 Error Result Object

- `test_error_result_object.cpp`
  - `result.error()` returns correct error code
  - `result.pos()` points to error location
  - Boolean conversion: `!result` for errors
  - Different error codes for different failures

---

## 9. C++ Type System Edge Cases

### 9.1 Zero-Sized Types

- `test_cpp_zero_sized.cpp`
  - `std::array<int, 0>` - empty array
  - Struct with no fields: `struct Empty {};`
  - Array of empty structs

### 9.2 Alignment and Padding

- `test_cpp_alignment.cpp`
  - Structs with padding between fields
  - Aligned types: `alignas(16) int x;`
  - Verify PFR handles correctly

### 9.3 Const and References

- `test_cpp_const_fields.cpp`
  - `const int x;` fields (if supported)
  - Behavior during parsing

### 9.4 Default Member Initializers

- `test_cpp_default_values.cpp`
  - Fields with default values: `int x = 42;`
  - Optional fields with defaults
  - Behavior when JSON field is absent

---

## 10. JSON Spec Compliance (RFC 8259)

### 10.1 Numbers

- `test_json_numbers_format.cpp`
  - Integer: `123`, `-123`, `0`
  - Decimal: `123.456`, `-123.456`
  - Exponent: `1e10`, `1E10`, `1e+10`, `1e-10`
  - Combined: `1.23e-4`
  - Leading zeros (should reject): `01`, `00.5`
  - Trailing decimal (should reject): `1.`
  - Leading decimal (should reject): `.5`

### 10.2 Strings

- `test_json_strings_unicode.cpp`
  - Basic ASCII
  - Unicode escapes: `\uXXXX`
  - Surrogate pairs: `\uD834\uDD1E`
  - All required escape sequences: `\"`, `\\`, `\/`, `\b`, `\f`, `\n`, `\r`, `\t`
  - Invalid escapes (should reject): `\x`, `\u123` (incomplete)

### 10.3 Structural Tokens

- `test_json_structure.cpp`
  - Proper nesting of `{}` and `[]`
  - Correct use of `,` and `:`
  - Empty object: `{}`
  - Empty array: `[]`

---

## 11. Performance & Limits

### 11.1 Deep Nesting

- `test_limits_nesting_depth.cpp`
  - 10 levels of nesting
  - 50 levels (if compiler supports)
  - Verify stack usage is bounded by C++ type depth

### 11.2 Large Arrays

- `test_limits_large_arrays.cpp`
  - `std::array<int, 1000>`
  - Verify constexpr evaluation succeeds
  - Compilation time acceptable

### 11.3 Many Fields

- `test_limits_many_fields.cpp`
  - Struct with 50+ fields
  - Verify PFR handles correctly
  - Compilation time acceptable

---

## 12. Integration Tests

### 12.1 Real-World Configs

- `test_integration_server_config.cpp`
  - Realistic server config structure
  - Mix of primitives, nested objects, arrays
  - Validation constraints

- `test_integration_sensor_data.cpp`
  - Embedded sensor data format
  - Fixed-size buffers
  - Timestamps, readings, metadata

- `test_integration_ui_state.cpp`
  - UI state serialization
  - Optional fields (not all UI elements present)
  - Nested components

### 12.2 Edge-to-Edge

- `test_integration_full_stack.cpp`
  - Parse → Process → Serialize → Parse
  - With streaming consumers/producers
  - With validation
  - With annotations

---

## Test Implementation Guidelines

### Each Test File Should:

1. **Include only necessary headers**
   ```cpp
   #include <JsonFusion/parser.hpp>  // C++23 required
   #include <string_view>
   #include <array>
   ```

2. **Define minimal test structures**
   ```cpp
   struct TestStruct {
       int field;
   };
   ```

3. **Use descriptive `static_assert` lambdas**
   ```cpp
   // Test: Positive integer parsing
   static_assert([]() constexpr {
       TestStruct s;
       return Parse(s, std::string_view(R"({"field": 42})"))
           && s.field == 42;
   }());
   ```

4. **Group related assertions**
   - 3-5 assertions per file typically
   - All testing the same feature/category

5. **Include comments for non-obvious tests**
   ```cpp
   // Per RFC 8259, leading zeros are not allowed
   static_assert(/* ... */);
   ```

### Naming Conventions

- `test_parse_<type>_<variant>.cpp` - Parsing tests
- `test_serialize_<type>_<variant>.cpp` - Serialization tests
- `test_validation_<constraint>_<type>.cpp` - Validation tests
- `test_annotated_<option>.cpp` - Annotation tests
- `test_error_<category>.cpp` - Error handling tests
- `test_roundtrip_<category>.cpp` - Parse→Serialize→Parse tests
- `test_json_<feature>.cpp` - JSON spec compliance tests
- `test_cpp_<feature>.cpp` - C++ type system tests
- `test_integration_<scenario>.cpp` - Real-world scenarios

---

## Priority Levels

### P0 - Critical (Implement First)
- All primitive types (integers, bool, char arrays)
- Basic nested structs
- Fixed-size arrays
- Optionals
- Basic serialization + round-trips
- Core validation (range, length, items)
- Key annotations

### P1 - High Priority
- Error handling (malformed JSON, type mismatches)
- JSON spec compliance (whitespace, escaping, numbers)
- Advanced annotations (as_array, not_json, not_required)
- Streaming consumers/producers
- Field ordering independence

### P2 - Medium Priority
- Edge cases (zero-sized, alignment, defaults)
- Deep nesting, large arrays
- Integration tests
- Less common integer types

### P3 - Nice to Have
- Performance tests (compilation time)
- Negative tests (compilation failures)
- Exhaustive Unicode testing
- Exotic type combinations

---

## Current Status Summary

### Implemented (5 tests)
- ✅ `test_parse_int.cpp` - Basic int parsing
- ✅ `test_parse_bool.cpp` - Boolean parsing
- ✅ `test_parse_char_array.cpp` - String parsing with null-termination
- ✅ `test_serialize_int.cpp` - Integer serialization
- ✅ `test_serialize_bool.cpp` - Boolean serialization

### Estimated Total: **150-200 test files**

This comprehensive coverage would:
- Verify every JsonFusion feature works at compile time
- Document capabilities through executable examples
- Catch regressions immediately
- Prove zero hidden runtime dependencies
- Serve as living documentation

---

## Next Steps

1. **Implement P0 tests** (~40 files)
2. **Set up CI integration** (compile all tests in parallel)
3. **Implement P1 tests** (~50 files)
4. **Add test coverage metrics** (track which features are tested)
5. **Implement P2 tests** (~40 files)
6. **Document learnings** (update main docs with constexpr examples)
7. **Implement P3 tests** (~20 files)

Each test takes ~5 minutes to write and provides permanent, zero-cost verification!

