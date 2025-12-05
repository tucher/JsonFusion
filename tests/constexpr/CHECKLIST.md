# Constexpr Test Implementation Checklist

Quick reference for tracking test implementation progress.

## ⚠️ Important: Floating-Point Limitations

**Floating-point types (`float`, `double`) are NOT constexpr-compatible in JsonFusion.**
- Tests marked with ⚠️ **NO FLOATING-POINT** or ⚠️ **RUNTIME ONLY** should use integer types only
- `range<>` validator: integer types only in constexpr tests
- `constant<>` validator: bool and integer types only (not float/double)
- `float_decimals<>` and `skip_materializing`: runtime-only features
- JSON number format tests: integer parsing only (no decimals/exponents)

## io/ (1 file) ✅ COMPLETE
- [x] test_io_iterators.cpp - Byte-by-byte streaming with custom iterators

## concepts/ (6 files) ✅ COMPLETE
- [x] test_json_parsable_value.cpp
- [x] test_json_serializable_value.cpp
- [x] test_consuming_streamer_like.cpp
- [x] test_producing_streamer_like.cpp
- [x] test_map_stdlib_interface.cpp
- [x] test_structural_detection.cpp

## primitives/ (12 files) ✅ COMPLETE
- [x] test_parse_int.cpp
- [x] test_parse_bool.cpp
- [x] test_parse_char_array.cpp
- [x] test_serialize_int.cpp
- [x] test_serialize_bool.cpp
- [x] test_parse_integers_signed.cpp
- [x] test_parse_integers_unsigned.cpp
- [x] test_parse_integers_overflow.cpp
- [x] test_parse_integers_edge_cases.cpp
- [x] test_parse_bool_edge_cases.cpp
- [x] test_parse_strings_escaping.cpp
- [x] test_parse_strings_edge_cases.cpp

## composite/ (10 files) ✅ COMPLETE
- [x] test_parse_nested_flat.cpp
- [x] test_parse_array_primitives.cpp
- [x] test_parse_array_nested.cpp
- [x] test_parse_optional_primitives.cpp
- [x] test_parse_optional_nested.cpp
- [x] test_parse_unique_ptr_primitives.cpp - `std::unique_ptr<T>` with primitives (null vs present)
- [x] test_parse_unique_ptr_nested.cpp - `std::unique_ptr<T>` with nested structs and arrays
- [x] test_parse_string.cpp
- [x] test_parse_vector_primitives.cpp
- [x] test_parse_vector_nested.cpp

## json_spec/ (6 files) - 5/6 COMPLETE
- [x] test_json_whitespace.cpp - Whitespace handling (RFC 8259)
- [x] test_json_field_order.cpp - Field ordering independence
- [x] test_json_null.cpp - Null handling (std::optional, std::unique_ptr)
- [x] test_json_syntax_valid.cpp - Valid JSON syntax (empty keys, deep nesting, large arrays)
- [x] test_json_syntax_invalid.cpp - Invalid JSON syntax error detection
- [x] test_json_strings_unicode.cpp - Unicode escapes, surrogate pairs, RFC 8259 compliance
- [ ] test_json_numbers_format.cpp - ⚠️ **INTEGER PARSING ONLY (no floating-point decimals/exponents)**

## validation/ (13 files) ✅ COMPLETE
- [x] test_map_validators.cpp - All map validators (45 tests)
  - `min_properties`, `max_properties`
  - `min_key_length`, `max_key_length`
  - `required_keys`, `allowed_keys`, `forbidden_keys`
- [x] test_string_enum.cpp - String enum validation (17 tests)
  - `enum_values<...>` with incremental validation
- [x] test_validation_constant.cpp - `constant<>` (bool, **integer only**), `string_constant<>` ⚠️ **NO FLOATING-POINT**
- [x] test_validation_range_int.cpp - `range<>` for integers ⚠️ **NO FLOATING-POINT**
- [x] test_validation_range_signed.cpp - `range<>` for all signed int types ⚠️ **NO FLOATING-POINT**
- [x] test_validation_range_unsigned.cpp - `range<>` for all unsigned int types ⚠️ **NO FLOATING-POINT**
- [x] test_validation_string_length.cpp - `min_length<>`, `max_length<>`
- [x] test_validation_array_items.cpp - `min_items<>`, `max_items<>`
- [x] test_validation_not_required.cpp - `not_required<>` (object-level)
- [x] test_validation_allow_excess_fields.cpp - `allow_excess_fields<>` (struct-level)
- [x] test_validation_combined_string.cpp - Multiple string validators
- [x] test_validation_combined_array.cpp - Multiple array validators
- [x] test_validation_combined_map.cpp - Multiple map validators

## annotations/ (9 files) - 4/9 COMPLETE
- [x] test_annotated_key.cpp - `key<"json_name">` remapping (covered in roundtrip tests)
- [x] test_annotated_as_array.cpp - `as_array` (struct as array) (covered in roundtrip tests)
- [ ] test_annotated_not_json.cpp - `not_json` (skip field) - NOT NEEDED NOW (covered in roundtrip tests)
- [x] test_annotated_skip_json.cpp - `skip_json<MaxSkipDepth>` (fast-skip)
- [x] test_annotated_json_sink.cpp - `json_sink<MaxSkipDepth, MaxStringLength>` (capture raw JSON)
- [ ] test_annotated_skip_materializing.cpp - `skip_materializing` (skip C++ work) ⚠️ **RUNTIME ONLY (floating-point)** - NOT NEEDED NOW
- [ ] test_annotated_float_decimals.cpp - `float_decimals<N>` (serialization precision) ⚠️ **RUNTIME ONLY (floating-point)** - NOT NEEDED NOW
- [ ] test_annotated_binary_fields_search.cpp - `binary_fields_search` (binary search optimization) - NOT NEEDED NOW
- [ ] test_annotated_description.cpp - `description<"text">` (metadata, optional) - NOT NEEDED NOW
- [ ] test_annotated_combinations.cpp - Multiple options together - NOT NEEDED NOW

## serialization/ (2 files)
- [x] test_serialize_int.cpp
- [x] test_serialize_bool.cpp

## roundtrip/ (3 files) ✅ COMPLETE
- [x] test_roundtrip_primitives.cpp
- [x] test_roundtrip_nested.cpp
- [x] test_roundtrip_annotated.cpp

## streaming/ (5 files) ✅ COMPLETE
- [x] test_map_streaming.cpp - Map consumer/producer streamers (basic coverage)
- [x] test_streamer_consumer_primitives.cpp - Array consumers for primitives (int, bool, char) - includes edge cases
- [x] test_streamer_map_consumer.cpp - Map consumers with std::array<char,N> and std::string keys
- [x] test_streamer_producer_primitives.cpp - Array producers for primitives
- [x] test_streamer_map_producer.cpp - Map producers with std::array<char,N> and std::string keys

## errors/ (7 files) - JSON Path Tracking ✅ COMPLETE
- [x] test_error_demo.cpp
- [x] test_error_json_path_primitives.cpp
- [x] test_error_json_path_arrays.cpp
- [x] test_error_json_path_maps.cpp
- [x] test_error_json_path_mixed.cpp
- [x] test_error_json_path_depth_calculation.cpp
- [x] test_error_path_annotations.cpp

## composite/ (edge cases - 5 files) - NOT STARTED
- [ ] test_parse_nested_complex.cpp
- [ ] test_parse_array_edge_cases.cpp
- [ ] test_parse_optional_edge_cases.cpp
- [ ] test_cpp_zero_sized.cpp
- [ ] test_cpp_default_values.cpp

## validation/ (additional - 5 files) - NOT STARTED
- [ ] test_validation_range_int.cpp
- [ ] test_validation_string_length.cpp
- [ ] test_validation_array_items.cpp
- [ ] test_validation_multiple_constraints.cpp
- [ ] test_validation_nested_objects.cpp

## annotations/ (9 files) - 2/9 COMPLETE
- [x] test_annotated_key.cpp - `key<"json_name">` remapping (covered in roundtrip tests)
- [x] test_annotated_as_array.cpp - `as_array` (struct as array) (covered in roundtrip tests)
- [ ] test_annotated_not_json.cpp - `not_json` (skip field) - NOT NEEDED NOW
- [x] test_annotated_skip_json.cpp - `skip_json<MaxSkipDepth>` (fast-skip)
- [x] test_annotated_json_sink.cpp - `json_sink<MaxSkipDepth, MaxStringLength>` (capture raw JSON)
- [ ] test_annotated_skip_materializing.cpp - `skip_materializing` (skip C++ work) ⚠️ **RUNTIME ONLY (floating-point)** - NOT NEEDED NOW
- [ ] test_annotated_float_decimals.cpp - `float_decimals<N>` (serialization precision) ⚠️ **RUNTIME ONLY (floating-point)** - NOT NEEDED NOW
- [ ] test_annotated_binary_fields_search.cpp - `binary_fields_search` (binary search optimization) - NOT NEEDED NOW
- [ ] test_annotated_description.cpp - `description<"text">` (metadata, optional) - NOT NEEDED NOW
- [ ] test_annotated_combinations.cpp - Multiple options together - NOT NEEDED NOW

## serialization/ (additional - 2 files) - NOT STARTED
- [ ] test_serialize_integers_all_types.cpp
- [ ] test_serialize_strings.cpp

## streaming/ (additional) ✅ COMPLETE
- [x] test_streamer_consumer_primitives.cpp - ✅ COMPLETE (includes empty arrays, single element, early termination)
- [x] test_streamer_map_consumer.cpp - ✅ COMPLETE (includes std::string keys)
- [x] test_streamer_producer_primitives.cpp - ✅ COMPLETE (includes empty, single, many elements)
- [x] test_streamer_map_producer.cpp - ✅ COMPLETE (includes std::string keys)

## errors/ (additional - 8 files) - NOT STARTED
- [ ] test_error_type_mismatch.cpp
- [ ] test_error_malformed_json.cpp
- [ ] test_error_validation_range.cpp
- [ ] test_error_validation_length.cpp
- [ ] test_error_string_overflow.cpp
- [ ] test_error_array_overflow.cpp
- [ ] test_error_result_object.cpp
- [ ] test_error_position_tracking.cpp

## limits/ (4 files) ✅ COMPLETE
- [x] test_limits_nesting_depth.cpp - Deep nesting (10 levels, arrays, optionals, mixed)
- [x] test_limits_large_arrays.cpp - Large arrays (100 ints, 50 bools, 20 strings, 10x10 matrix, 50 in struct)
- [x] test_limits_many_fields.cpp - Many fields (50 fields, 30 mixed, nested, array fields)
- [x] test_limits_many_map_keys.cpp - Many map keys (30 allowed, 20 required, 30 forbidden, combined, binary search threshold)

---

**Progress: 53 / ~90 tests implemented (~58.9%)**



**Complete Coverage Checklist:**

**Validators (17 total):**
- ✅ `enum_values<>` - String enum validation (test_string_enum.cpp)
- ✅ `min_properties<>`, `max_properties<>` - Map property count (test_map_validators.cpp)
- ✅ `min_key_length<>`, `max_key_length<>` - Map key length (test_map_validators.cpp)
- ✅ `required_keys<>`, `allowed_keys<>`, `forbidden_keys<>` - Map key sets (test_map_validators.cpp)
- ✅ `constant<>` - Bool/**integer** constants ⚠️ **NO FLOATING-POINT** (test_validation_constant.cpp)
- ✅ `string_constant<>` - String constants (test_validation_constant.cpp)
- ✅ `range<>` - Number range validation ⚠️ **INTEGER ONLY (no float/double)** (test_validation_range_int.cpp, test_validation_range_signed.cpp, test_validation_range_unsigned.cpp)
- ✅ `min_length<>`, `max_length<>` - String length (test_validation_string_length.cpp)
- ✅ `min_items<>`, `max_items<>` - Array item count (test_validation_array_items.cpp)
- ✅ `not_required<>` - Object-level optional fields (test_validation_not_required.cpp)
- ✅ `allow_excess_fields<>` - Allow unknown JSON fields (test_validation_allow_excess_fields.cpp)

**Options (9 total):**
- ✅ `key<>` - JSON key remapping (covered in roundtrip tests)
- ✅ `as_array` - Struct as array (covered in roundtrip tests)
- ✅ `not_json` - Skip field (covered in roundtrip tests)
- ✅ `skip_json<>` - Fast-skip JSON value (test_annotated_skip_json.cpp)
- ✅ `json_sink<>` - Capture raw JSON as string or fixed-sized string-like array (test_annotated_json_sink.cpp)
- [ ] `skip_materializing` - Skip C++-side work **NOT NEEDED NOW**
- [ ] `float_decimals<>` - Serialization precision ⚠️ **RUNTIME ONLY (floating-point)** - **NOT NEEDED NOW**
- [ ] `binary_fields_search` - Binary search optimization **NOT NEEDED NOW**
- [ ] `description<>` - Metadata (optional tests) **NOT NEEDED NOW**

**Completed Categories:**
- ✅ io/ (1/1)
- ✅ concepts/ (6/6)
- ✅ primitives/ (12/12)
- ✅ composite/ (10/10) - Includes unique_ptr tests
- ✅ roundtrip/ (3/3)
- ✅ errors/ JSON path tracking (7/7)
- ✅ json_spec/ (5/6) - RFC 8259 compliance (Unicode complete, numbers pending)
- ✅ validation/ (13/13) - All validators covered (constant, range, length, items, not_required, allow_excess_fields, map validators, combined)
- ✅ annotations/ (4/9) - key, as_array (roundtrip), skip_json, json_sink
- ✅ limits/ (4/4) - Nesting depth, large arrays, many fields, many map keys

**In Progress:**
- serialization/ (2/2 basic, more needed)
- validation/ ✅ COMPLETE - All validators covered (12 files)
- streaming/ ✅ COMPLETE - All core streaming functionality covered (primitives and maps)

**Priority P0 (Critical)**: Mostly complete
**Priority P1 (High)**: ~15 remaining
**Priority P2 (Medium)**: ~20 remaining

---

## Quick Add Commands

```bash
# Create a new test from template (replace CATEGORY with primitives, errors, etc.)
cat > CATEGORY/test_name.cpp << 'EOF'
#include "test_helpers.hpp"
using namespace TestHelpers;

struct TestStruct {
    int field;
};

// Tests
static_assert(TestParse(R"({"field": 42})", TestStruct{42}));
EOF

# Run single test
g++ -std=c++23 -I../../include -Itests/constexpr -c CATEGORY/test_name.cpp

# Run all tests
./run_tests.sh
```

