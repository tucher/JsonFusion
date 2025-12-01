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

## composite/ (8 files) ✅ COMPLETE
- [x] test_parse_nested_flat.cpp
- [x] test_parse_array_primitives.cpp
- [x] test_parse_array_nested.cpp
- [x] test_parse_optional_primitives.cpp
- [x] test_parse_optional_nested.cpp
- [x] test_parse_string.cpp
- [x] test_parse_vector_primitives.cpp
- [x] test_parse_vector_nested.cpp

## json_spec/ (6 files)
- [ ] test_json_whitespace.cpp
- [ ] test_json_field_order.cpp
- [ ] test_json_syntax_valid.cpp
- [ ] test_json_syntax_invalid.cpp
- [ ] test_json_numbers_format.cpp
- [ ] test_json_strings_unicode.cpp

## validation/ (2 files) - Basic coverage complete
- [x] test_map_validators.cpp - All map validators (45 tests)
  - `min_properties`, `max_properties`
  - `min_key_length`, `max_key_length`
  - `required_keys`, `allowed_keys`, `forbidden_keys`
- [x] test_string_enum.cpp - String enum validation (17 tests)
  - `enum_values<...>` with incremental validation

**Additional validation tests needed:**
- [ ] test_validation_constant.cpp - `constant<>` (bool, **integer only**), `string_constant<>` ⚠️ **NO FLOATING-POINT**
- [ ] test_validation_range_int.cpp - `range<>` for integers ⚠️ **NO FLOATING-POINT**
- [ ] test_validation_range_signed.cpp - `range<>` for all signed int types ⚠️ **NO FLOATING-POINT**
- [ ] test_validation_range_unsigned.cpp - `range<>` for all unsigned int types ⚠️ **NO FLOATING-POINT**
- [ ] test_validation_string_length.cpp - `min_length<>`, `max_length<>`
- [ ] test_validation_array_items.cpp - `min_items<>`, `max_items<>`
- [ ] test_validation_not_required.cpp - `not_required<>` (object-level)
- [ ] test_validation_combined_string.cpp - Multiple string validators
- [ ] test_validation_combined_array.cpp - Multiple array validators
- [ ] test_validation_combined_map.cpp - Multiple map validators

## annotations/ (6 files)
- [ ] test_annotated_key.cpp
- [ ] test_annotated_as_array.cpp
- [ ] test_annotated_not_json.cpp
- [ ] test_annotated_not_required.cpp
- [ ] test_annotated_allow_excess.cpp
- [ ] test_annotated_combinations.cpp

## serialization/ (2 files)
- [x] test_serialize_int.cpp
- [x] test_serialize_bool.cpp

## roundtrip/ (3 files) ✅ COMPLETE
- [x] test_roundtrip_primitives.cpp
- [x] test_roundtrip_nested.cpp
- [x] test_roundtrip_annotated.cpp

## streaming/ (1 file)
- [x] test_map_streaming.cpp - Map consumer/producer streamers

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

## json_spec/ (6 files) - NOT STARTED
- [ ] test_json_whitespace.cpp
- [ ] test_json_field_order.cpp
- [ ] test_json_syntax_valid.cpp
- [ ] test_json_syntax_invalid.cpp
- [ ] test_json_numbers_format.cpp - ⚠️ **INTEGER PARSING ONLY (no floating-point decimals/exponents)**
- [ ] test_json_strings_unicode.cpp

## validation/ (additional - 5 files) - NOT STARTED
- [ ] test_validation_range_int.cpp
- [ ] test_validation_string_length.cpp
- [ ] test_validation_array_items.cpp
- [ ] test_validation_multiple_constraints.cpp
- [ ] test_validation_nested_objects.cpp

## annotations/ (11 files) - NOT STARTED
- [ ] test_annotated_key.cpp - `key<"json_name">` remapping
- [ ] test_annotated_as_array.cpp - `as_array` (struct as array)
- [ ] test_annotated_not_json.cpp - `not_json` (skip field)
- [ ] test_annotated_not_required.cpp - `not_required<"field1", ...>` (object-level)
- [ ] test_annotated_allow_excess.cpp - `allow_excess_fields<>`
- [ ] test_annotated_skip_json.cpp - `skip_json<MaxSkipDepth>` (fast-skip)
- [ ] test_annotated_json_sink.cpp - `json_sink<MaxSkipDepth, MaxStringLength>` (capture raw JSON)
- [ ] test_annotated_skip_materializing.cpp - `skip_materializing` (skip C++ work) ⚠️ **RUNTIME ONLY (floating-point)**
- [ ] test_annotated_float_decimals.cpp - `float_decimals<N>` (serialization precision) ⚠️ **RUNTIME ONLY (floating-point)**
- [ ] test_annotated_binary_fields_search.cpp - `binary_fields_search` (binary search optimization)
- [ ] test_annotated_description.cpp - `description<"text">` (metadata, optional)
- [ ] test_annotated_combinations.cpp - Multiple options together

## serialization/ (additional - 2 files) - NOT STARTED
- [ ] test_serialize_integers_all_types.cpp
- [ ] test_serialize_strings.cpp

## streaming/ (additional - 5 files) - NOT STARTED
- [ ] test_streamer_consumer_primitives.cpp
- [ ] test_streamer_consumer_structs.cpp
- [ ] test_streamer_consumer_edge_cases.cpp
- [ ] test_streamer_producer_primitives.cpp
- [ ] test_streamer_producer_structs.cpp
- [ ] test_streamer_producer_nested.cpp

## errors/ (additional - 8 files) - NOT STARTED
- [ ] test_error_type_mismatch.cpp
- [ ] test_error_malformed_json.cpp
- [ ] test_error_validation_range.cpp
- [ ] test_error_validation_length.cpp
- [ ] test_error_string_overflow.cpp
- [ ] test_error_array_overflow.cpp
- [ ] test_error_result_object.cpp
- [ ] test_error_position_tracking.cpp

## limits/ (5 files) - NOT STARTED
- [ ] test_limits_nesting_depth.cpp
- [ ] test_limits_large_arrays.cpp
- [ ] test_limits_many_fields.cpp
- [ ] test_limits_many_validators.cpp
- [ ] test_limits_constexpr_steps.cpp

## integration/ (3 files) - NOT STARTED
- [ ] test_integration_server_config.cpp
- [ ] test_integration_sensor_data.cpp
- [ ] test_integration_full_stack.cpp

---

**Progress: 27 / ~90 tests implemented (~30%)**

**Complete Coverage Checklist:**

**Validators (16 total):**
- ✅ `enum_values<>` - String enum validation (test_string_enum.cpp)
- ✅ `min_properties<>`, `max_properties<>` - Map property count (test_map_validators.cpp)
- ✅ `min_key_length<>`, `max_key_length<>` - Map key length (test_map_validators.cpp)
- ✅ `required_keys<>`, `allowed_keys<>`, `forbidden_keys<>` - Map key sets (test_map_validators.cpp)
- [ ] `constant<>` - Bool/**integer** constants ⚠️ **NO FLOATING-POINT**
- [ ] `string_constant<>` - String constants
- [ ] `range<>` - Number range validation ⚠️ **INTEGER ONLY (no float/double)**
- [ ] `min_length<>`, `max_length<>` - String length
- [ ] `min_items<>`, `max_items<>` - Array item count
- [ ] `not_required<>` - Object-level optional fields

**Options (11 total):**
- ✅ `key<>` - JSON key remapping (covered in roundtrip tests)
- ✅ `as_array` - Struct as array (covered in roundtrip tests)
- ✅ `not_json` - Skip field (covered in roundtrip tests)
- [ ] `not_required<>` - Object-level optional fields
- [ ] `allow_excess_fields<>` - Allow extra JSON fields
- [ ] `skip_json<>` - Fast-skip JSON value
- [ ] `json_sink<>` - Capture raw JSON as string
- [ ] `skip_materializing` - Skip C++-side work
- [ ] `float_decimals<>` - Serialization precision ⚠️ **RUNTIME ONLY (floating-point)**
- [ ] `binary_fields_search` - Binary search optimization
- [ ] `description<>` - Metadata (optional tests)

**Completed Categories:**
- ✅ io/ (1/1)
- ✅ concepts/ (6/6)
- ✅ primitives/ (12/12)
- ✅ composite/ (8/8)
- ✅ roundtrip/ (3/3)
- ✅ errors/ JSON path tracking (7/7)

**In Progress:**
- serialization/ (2/2 basic, more needed)
- validation/ (2/2 basic, more needed)
- streaming/ (1/1 basic, more needed)

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

