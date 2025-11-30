# JsonFusion Constexpr Test Coverage Plan

This document outlines comprehensive test coverage for JsonFusion's compile-time guarantees. Since constexpr tests are fast to write and provide strong guarantees, we aim for exhaustive coverage.

## Organization Principles

- **One test per file** - Fast iteration, clear failures, parallel compilation
- **Multiple `static_assert`s per file** - Related test cases grouped together
- **Naming convention**: `test_<category>_<feature>.cpp`
- **Each test is self-contained** - Minimal dependencies, clear purpose
- **Streaming for complex types** - Use streamers for types with limited constexpr support (e.g., `std::map`)
- **Incremental validation testing** - Verify early rejection paths work correctly
- **Comprehensive validator coverage** - Test each validator individually and in combination

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

### 2.4 Dynamic Containers (`std::string`, `std::vector<T>`)

**Status**: ✅ Constexpr-compatible in C++23!

- `test_parse_string.cpp`
  - `std::string` with various lengths
  - Empty string
  - Very long strings
  - Strings with special characters
  - Round-trip: parse → serialize → parse

- `test_parse_vector_primitives.cpp`
  - `std::vector<int>` with various sizes
  - `std::vector<bool>`
  - Empty vectors
  - Large vectors (100+ elements)

- `test_parse_vector_nested.cpp`
  - `std::vector<NestedStruct>`
  - `std::vector<std::vector<int>>` (2D)
  - `std::vector<std::string>`
  - Mixed with fixed arrays

- `test_parse_vector_edge_cases.cpp`
  - Vector growth during parsing
  - Memory allocation in constexpr
  - Vector with optional elements: `std::vector<std::optional<int>>`

### 2.5 Map Types (`std::map`, `std::unordered_map`)

**Status**: ⚠️ `std::map` limited constexpr support; use streamers for testing

- `test_parse_map_basic.cpp`
  - `std::map<std::string, int>`
  - `std::map<std::string, NestedStruct>`
  - Empty maps
  - Single entry
  - Multiple entries

- `test_parse_map_key_types.cpp`
  - `std::array<char, N>` keys (fixed-size)
  - `std::string` keys (dynamic)
  - Key ordering (sorted for `std::map`)

- `test_parse_map_value_types.cpp`
  - Primitive values: int, bool, string
  - Struct values
  - Array values: `std::map<std::string, std::array<int, 3>>`
  - Optional values: `std::map<std::string, std::optional<int>>`

- `test_parse_map_nested.cpp`
  - Nested maps: `std::map<std::string, std::map<std::string, int>>`
  - Maps in structs
  - Maps in arrays
  - Structs containing maps

- `test_parse_map_edge_cases.cpp`
  - Duplicate keys (error detection)
  - Keys with special characters
  - Very long keys
  - Many entries (performance)

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

### 4.1 Primitive Validators

- `test_validation_constant.cpp`
  - `constant<42>` - bool constants
  - Rejects non-matching values
  - With different primitive types

### 4.2 Number Range Validation

- `test_validation_range_int.cpp`
  - `Annotated<int, range<0, 100>>` - valid values at boundaries
  - Below min, above max (error detection)
  - Exactly at min, exactly at max
  - Negative ranges: `range<-100, -10>`
  - Single-value range: `range<42, 42>`

- `test_validation_range_signed.cpp`
  - All signed integer types: `int8_t`, `int16_t`, `int32_t`, `int64_t`
  - Type-specific min/max values
  - Overflow detection

- `test_validation_range_unsigned.cpp`
  - All unsigned integer types
  - Zero boundaries
  - Max value boundaries

### 4.3 String Validators

**Status**: ✅ `test_parse_strings_escaping.cpp` partially covers this

- `test_validation_string_length.cpp`
  - `min_length<N>`, `max_length<N>`
  - Empty string with `min_length<1>` (error)
  - Exactly at boundaries
  - Both constraints together: `min_length<5>, max_length<10>`
  - Early rejection with `max_length` (incremental validation)

- `test_validation_string_enum.cpp`
  - `enum_values<"red", "green", "blue">`
  - Valid enum values
  - Invalid value rejection
  - Empty string rejection
  - Case sensitivity
  - Prefix/partial matches (should reject)
  - Many values (10+) - binary search
  - In structs, arrays, with other validators

### 4.4 Array/Vector Validators

- `test_validation_array_items.cpp`
  - `min_items<N>`, `max_items<N>`
  - Empty array constraints
  - Exactly at boundaries
  - Both constraints together
  - With `std::array<T, N>` and `std::vector<T>`
  - Early rejection with `max_items`

### 4.5 Object Validators (not_required)

- `test_validation_not_required.cpp`
  - `not_required<"field1", "field2">` at object level
  - Field can be absent from JSON
  - All fields absent
  - Some required, some not
  - Interaction with `std::optional`
  - Nested objects with different requirements
  - Error when required field is missing

### 4.6 Map Property Count Validators

- `test_validation_map_properties.cpp`
  - `min_properties<N>`, `max_properties<N>`
  - Empty maps
  - Exactly at boundaries
  - Both constraints together
  - Early rejection with `max_properties`

### 4.7 Map Key Validators

- `test_validation_map_key_length.cpp`
  - `min_key_length<N>`, `max_key_length<N>`
  - All keys within bounds
  - One key violates constraint
  - Exact boundaries
  - Both constraints together

- `test_validation_map_required_keys.cpp`
  - `required_keys<"id", "name">`
  - All required keys present
  - One or more missing
  - Extra keys beyond required
  - Empty map with required keys (error)
  - Incremental tracking with `std::bitset`

- `test_validation_map_allowed_keys.cpp`
  - `allowed_keys<"x", "y", "z">`
  - All keys in whitelist
  - One key not allowed (error)
  - Early rejection on invalid prefix
  - Empty allowed list (rejects all)
  - Many allowed keys (20+) - binary search

- `test_validation_map_forbidden_keys.cpp`
  - `forbidden_keys<"__proto__", "constructor">`
  - No forbidden keys present
  - One forbidden key (error)
  - Similar but non-matching keys (e.g., "badge" vs "bad")
  - Empty forbidden list (allows all)

### 4.8 Combined Validators

- `test_validation_combined_string.cpp`
  - `min_length` + `max_length` + `enum_values`
  - Multiple constraints, all pass
  - One constraint fails

- `test_validation_combined_array.cpp`
  - `min_items` + `max_items` with element validators
  - Nested validation

- `test_validation_combined_map.cpp`
  - `min_properties` + `max_properties` + `required_keys` + `allowed_keys`
  - All three key validators together
  - `allowed_keys` + `forbidden_keys` (forbidden takes precedence)
  - Key length + key set validators

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

### 5.4 not_required (Object-Level Annotation)

- `test_annotated_not_required.cpp`
  - Annotated at object level: `Annotated<MyStruct, not_required<"field1", "field2">>`
  - Specified fields can be absent from JSON
  - Default values are used for absent fields
  - Present vs absent behavior
  - All fields not_required
  - Mix of required and not_required fields
  - Difference from `std::optional` (field-level)
  - Nested objects with different not_required annotations

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

### 7.3 Map Streamers

**Status**: ✅ `test_map_streaming.cpp` (basic coverage)

- `test_streamer_map_consumer.cpp`
  - `ConsumingMapStreamerLike` concept verification
  - Consumer with key-value pairs
  - `value_type = MapEntry<K, V>`
  - `consume()`, `finalize()`, `reset()` lifecycle
  - Duplicate key detection in consumer
  - Empty map
  - Single entry
  - Many entries

- `test_streamer_map_producer.cpp`
  - `ProducingMapStreamerLike` concept verification
  - Producer generating key-value pairs
  - Lifecycle with `reset()`, `read_key()`, `read_value()`
  - Nested maps via producers
  - Different key/value types

- `test_streamer_map_with_validators.cpp`
  - Map streamers with `min_properties`, `max_properties`
  - Map streamers with `required_keys`, `allowed_keys`, `forbidden_keys`
  - Map streamers with key length constraints
  - Validation failures during streaming

### 7.4 Streamer Concepts

- `test_streamer_concepts.cpp`
  - `ConsumingStreamerLike` concept verification
  - `ProducingStreamerLike` concept verification
  - `ConsumingMapStreamerLike` concept verification
  - `ProducingMapStreamerLike` concept verification
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
  - **Verify JSON path points to correct location for each error**

### 8.2 Malformed JSON

- `test_error_malformed_json.cpp`
  - Unclosed braces, brackets
  - Missing quotes on strings
  - Invalid escape sequences
  - Truncated JSON
  - Invalid UTF-8 (if applicable)
  - **Verify error offset and JSON path for each malformed input**

### 8.3 Validation Failures

- `test_error_validation_range.cpp`
  - Value outside `range<>`
  - `result.validationResult()` contains schema errors
  - `result.validationResult().hasSchemaError(SchemaError::number_out_of_range)`
  - Parse error vs validation error distinction
  - **Verify `result.errorJsonPath()` shows correct field path (e.g., `$.config.port`)**

- `test_error_validation_length.cpp`
  - String too short/long
  - Array too short/long
  - `SchemaError::string_length_out_of_range`
  - `SchemaError::array_items_count_out_of_range`
  - **Verify JSON path for nested validation errors**

- `test_error_validation_map.cpp`
  - `SchemaError::map_properties_count_out_of_range`
  - `SchemaError::map_key_length_out_of_range`
  - `SchemaError::map_key_not_allowed`
  - `SchemaError::map_key_forbidden`
  - `SchemaError::map_missing_required_key`
  - **Verify JSON path includes map key context**

- `test_error_validation_enum.cpp`
  - String not in `enum_values`
  - `SchemaError::wrong_constant_value`
  - Early rejection during parsing
  - **Verify JSON path to enum field**

### 8.4 Buffer Overflow

- `test_error_string_overflow.cpp`
  - String longer than `std::array<char, N>`
  - Behavior: truncate, error, or undefined?
  - **JSON path should point to the overflowing string field**

- `test_error_array_overflow.cpp`
  - JSON array longer than `std::array<T, N>`
  - **JSON path includes array index where overflow occurred**

### 8.5 Error Result Object

- `test_error_result_object.cpp`
  - Parse errors: `result.error()` returns `ParseError` enum
  - Parse error position: `result.offset()` points to error location in byte stream
  - Boolean conversion: `!result` for errors, `result` for success
  - Different error codes for different parse failures

- `test_error_validation_result.cpp`
  - Validation errors: `result.validationResult()` returns `ValidationResult`
  - `result.validationResult().schema_errors()` returns bitmask
  - `result.validationResult().hasSchemaError(SchemaError::...)` checks specific error
  - `ValidationResult` boolean conversion
  - Parse succeeds but validation fails
  - Both parse and validation errors

- `test_error_result_combinations.cpp`
  - Success: no parse error, no validation error
  - Parse error only (malformed JSON)
  - Validation error only (well-formed but invalid)
  - Error position tracking with validation errors

### 8.6 JSON Path Tracking

- `test_error_json_path_primitives.cpp`
  - Error in primitive field: `$.field`
  - Error in nested object field: `$.outer.inner.field`
  - Error in array element: `$.items[3]`
  - Error in deeply nested structure: `$.a.b.c.d.e`
  - Path for root-level errors: `$`

- `test_error_json_path_arrays.cpp`
  - Error in first array element: `$.array[0]`
  - Error in middle element: `$.array[5]`
  - Error in nested array: `$.matrix[2][3]`
  - Error in array of objects: `$.users[10].name`
  - 3D array paths: `$.tensor[1][2][3]`

- `test_error_json_path_maps.cpp`
  - Error in map value: `$.config["server"]`
  - Error in nested map: `$.config["db"]["host"]`
  - Map key validation error shows key context
  - Dynamic map keys vs struct field names

- `test_error_json_path_mixed.cpp`
  - Complex paths: `$.statuses[3].user.entities.urls[0].display_url`
  - Arrays of maps: `$.items[5]["metadata"]`
  - Maps of arrays: `$.groups["admins"][2]`
  - Annotated fields with `key<>` remapping show JSON key, not C++ name

- `test_error_json_path_depth_calculation.cpp`
  - Compile-time depth calculation: `calc_type_depth<Type>()`
  - Simple flat struct: depth = 1
  - Nested struct: depth = max(field depths) + 1
  - Array increases depth: `vector<T>` → depth(T) + 1
  - Recursive type detection with `SeenTypes` tracking
  - Cyclic recursive types return `SCHEMA_UNBOUNDED`

- `test_error_json_path_storage.cpp`
  - `JsonStaticPath<N>` for non-cyclic schemas
  - Stack-allocated, compile-time sized from `calc_type_depth`
  - `JsonDynamicPath` for cyclic schemas (with macro flag)
  - Path operations: `push_child()`, `pop()`, `currentLength`
  - `PathElement` structure: `array_index`, `field_name`

- `test_error_json_path_constexpr.cpp`
  - JSON path tracking works in constexpr parsing
  - Verify path at compile time in `static_assert`
  - Example: `result.errorJsonPath().storage[1].field_name == "name"`
  - Constexpr path depth calculation
  - Zero runtime allocation overhead

- `test_error_json_path_formatting.cpp`
  - Path to string conversion: `ParseResultToString()`
  - Format: `$.field`, `$.array[3]`, `$.obj.nested`
  - Context window around error: `...before😖after...`
  - Integration with `error_formatting.hpp`
  - Human-readable error messages

### 8.7 Error Path Correctness

- `test_error_path_validation.cpp`
  - Every error type has correct JSON path
  - Path matches actual location in JSON
  - Nested errors at correct depth
  - Array indices are accurate
  - Map keys are tracked correctly

- `test_error_path_incremental.cpp`
  - Path is maintained during incremental parsing
  - Path updates as parser descends into structures
  - Path restored correctly on backtracking (if applicable)
  - Early validation rejection has correct path

- `test_error_path_annotations.cpp`
  - Fields with `key<"json_name">` show JSON name in path
  - `as_array` structures show field names, not array indices (JSON path reflects C++ structure)
  - `not_json` fields don't appear in paths
  - Optional fields show in path when present

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

### 11.4 Validator Performance

- `test_limits_many_validators.cpp`
  - Multiple validators on same field
  - 5+ validators combined
  - Compilation time impact
  - Zero runtime overhead

- `test_limits_map_many_keys.cpp`
  - `allowed_keys` with 50+ keys
  - Binary search vs linear search threshold
  - `required_keys` with many keys
  - `std::bitset` size limits

### 11.5 Constexpr Evaluation Limits

- `test_limits_constexpr_steps.cpp`
  - Complex nested structures in constexpr
  - Large data structures (1000+ elements)
  - `-fconstexpr-steps` requirements
  - Incremental validation reduces constexpr steps

---

## 12. Integration Tests

### 12.1 Real-World Configs

- `test_integration_server_config.cpp`
  - Realistic server config structure
  - Mix of primitives, nested objects, arrays, maps
  - Validation constraints: `range`, `enum_values`, `required_keys`
  - Environment-specific configs with `enum_values<"dev", "staging", "prod">`
  - Connection pools with min/max constraints

- `test_integration_sensor_data.cpp`
  - Embedded sensor data format
  - Fixed-size buffers (`std::array`)
  - Timestamps, readings, metadata
  - Arrays of sensor readings
  - Validation: `range` for sensor values

- `test_integration_ui_state.cpp`
  - UI state serialization
  - Optional fields (not all UI elements present)
  - Nested components
  - Maps for dynamic UI elements: `std::map<std::string, UIComponent>`

- `test_integration_api_response.cpp`
  - REST API response models
  - Status codes with `enum_values`
  - Headers as maps: `std::map<std::string, std::string>`
  - Nested data payload
  - Error responses with validation

- `test_integration_json_schema.cpp`
  - Implementing JSON Schema patterns
  - Complex validation rules
  - `required_keys` + `allowed_keys` + `forbidden_keys`
  - `min`/`max` constraints on numbers, strings, arrays, maps
  - Enum values for restricted strings

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

### P0 - Critical (Mostly Complete ✅)
- ✅ All primitive types (integers, bool, char arrays)
- ✅ Basic nested structs
- ✅ Fixed-size arrays
- ✅ Optionals
- ✅ Basic serialization + round-trips
- ✅ Core validation (range, length, items)
- ✅ Map support (parse, serialize, validate)
- ✅ String enum validation
- ✅ Map key validators (required, allowed, forbidden)
- ⚠️ Key annotations (partial)

### P1 - High Priority (Partially Complete)
- ⚠️ Error handling (malformed JSON, type mismatches)
- ✅ JSON spec compliance (whitespace, escaping partially done)
- ⚠️ Advanced annotations (as_array, not_json, not_required)
- ✅ Streaming consumers/producers (including maps)
- ⚠️ Field ordering independence
- 🔲 `std::string` and `std::vector` comprehensive testing
- 🔲 Round-trip tests for all types

### P2 - Medium Priority (To Do)
- 🔲 Edge cases (zero-sized, alignment, defaults)
- 🔲 Deep nesting, large arrays
- 🔲 Integration tests (real-world configs)
- 🔲 Less common integer types
- 🔲 Validation error result object testing
- 🔲 Combined validators testing

### P3 - Nice to Have
- 🔲 Performance tests (compilation time)
- 🔲 Negative tests (compilation failures)
- 🔲 Exhaustive Unicode testing
- 🔲 Exotic type combinations
- 🔲 Constexpr limits exploration

---

## Current Status Summary

### Implemented Tests
**Primitives**:
- ✅ `test_parse_int.cpp` - Basic int parsing
- ✅ `test_parse_bool.cpp` - Boolean parsing
- ✅ `test_parse_char_array.cpp` - String parsing with null-termination
- ✅ `test_parse_integers_overflow.cpp` - Integer overflow detection
- ✅ `test_parse_strings_escaping.cpp` - String escape sequences

**Serialization**:
- ✅ `test_serialize_int.cpp` - Integer serialization
- ✅ `test_serialize_bool.cpp` - Boolean serialization

**Streaming**:
- ✅ `test_map_streaming.cpp` - Map consumer/producer streamers

**Validation**:
- ✅ `test_map_validators.cpp` - All map validators (45 tests)
  - `min_properties`, `max_properties`
  - `min_key_length`, `max_key_length`
  - `required_keys`, `allowed_keys`, `forbidden_keys`
- ✅ `test_string_enum.cpp` - String enum validation (17 tests)
  - `enum_values<...>` with incremental validation

### Key Achievements
- ✅ **Map support**: Full map parsing, serialization, and validation
- ✅ **Incremental validation**: Early rejection for performance
- ✅ **Stateful validators**: Generic state mechanism for complex validation
- ✅ **Adaptive search**: Binary/linear search selection at compile time
- ✅ **String enums**: Comprehensive enum validation with early rejection

### Estimated Total: **180-220 test files**

This comprehensive coverage would:
- Verify every JsonFusion feature works at compile time
- Document capabilities through executable examples
- Catch regressions immediately
- Prove zero hidden runtime dependencies
- Serve as living documentation
- Cover 95%+ of typical C++ JSON use cases

---

## Next Steps

### Immediate (Complete P0)
1. **Test `std::string` and `std::vector`** (~8 files)
   - Basic parsing, nested, edge cases
   - Serialization and round-trips
2. **Complete primitive coverage** (~5 files)
   - All integer types systematically
   - More edge cases

### Short Term (P1 Priority)
3. **Error handling tests** (~10 files)
   - Malformed JSON detection
   - Type mismatch errors
   - Validation error result object
4. **Round-trip tests** (~8 files)
   - All types: primitives, structs, arrays, maps
   - With validators, annotations
5. **JSON spec compliance** (~8 files)
   - Number formats, string escaping
   - Whitespace, syntax edge cases

### Medium Term (P2 Priority)
6. **Integration tests** (~5 files)
   - Real-world config files
   - API responses
   - JSON Schema patterns
7. **Combined validator tests** (~5 files)
   - Multiple validators together
   - Complex validation scenarios
8. **Set up CI integration** (compile all tests in parallel)

### Long Term (P3 & Documentation)
9. **Performance & limits tests** (~10 files)
10. **Document learnings** (update main docs with constexpr examples)
11. **Add test coverage metrics** (track which features are tested)

**Progress**: ~12/200 files complete (~6%)
Each test takes ~5-10 minutes to write and provides permanent, zero-cost verification!

