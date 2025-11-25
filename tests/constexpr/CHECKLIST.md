# Constexpr Test Implementation Checklist

Quick reference for tracking test implementation progress.

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

## composite/ (8 files)
- [ ] test_parse_nested_flat.cpp
- [ ] test_parse_nested_complex.cpp
- [ ] test_parse_array_primitives.cpp
- [ ] test_parse_array_nested.cpp
- [ ] test_parse_array_edge_cases.cpp
- [ ] test_parse_optional_primitives.cpp
- [ ] test_parse_optional_nested.cpp
- [ ] test_parse_optional_edge_cases.cpp

## json_spec/ (6 files)
- [ ] test_json_whitespace.cpp
- [ ] test_json_field_order.cpp
- [ ] test_json_syntax_valid.cpp
- [ ] test_json_syntax_invalid.cpp
- [ ] test_json_numbers_format.cpp
- [ ] test_json_strings_unicode.cpp

## validation/ (5 files)
- [ ] test_validation_range_int.cpp
- [ ] test_validation_string_length.cpp
- [ ] test_validation_array_items.cpp
- [ ] test_validation_multiple_constraints.cpp
- [ ] test_validation_nested_objects.cpp

## annotations/ (6 files)
- [ ] test_annotated_key.cpp
- [ ] test_annotated_as_array.cpp
- [ ] test_annotated_not_json.cpp
- [ ] test_annotated_not_required.cpp
- [ ] test_annotated_allow_excess.cpp
- [ ] test_annotated_combinations.cpp

## serialization/ (5 files)
- [x] test_serialize_int.cpp
- [x] test_serialize_bool.cpp
- [ ] test_serialize_integers_all_types.cpp
- [ ] test_serialize_strings.cpp
- [ ] test_roundtrip_primitives.cpp
- [ ] test_roundtrip_nested.cpp
- [ ] test_roundtrip_annotated.cpp

## streaming/ (6 files)
- [ ] test_streamer_consumer_primitives.cpp
- [ ] test_streamer_consumer_structs.cpp
- [ ] test_streamer_consumer_edge_cases.cpp
- [ ] test_streamer_producer_primitives.cpp
- [ ] test_streamer_producer_structs.cpp
- [ ] test_streamer_producer_nested.cpp

## errors/ (8 files)
- [x] test_error_demo.cpp
- [ ] test_error_type_mismatch.cpp
- [ ] test_error_malformed_json.cpp
- [ ] test_error_validation_range.cpp
- [ ] test_error_validation_length.cpp
- [ ] test_error_string_overflow.cpp
- [ ] test_error_array_overflow.cpp
- [ ] test_error_result_object.cpp
- [ ] test_error_position_tracking.cpp

## composite/ (edge cases - 5 files)
- [ ] test_cpp_zero_sized.cpp
- [ ] test_cpp_default_values.cpp
- [ ] test_limits_nesting_depth.cpp
- [ ] test_limits_large_arrays.cpp
- [ ] test_limits_many_fields.cpp

## integration/ (3 files)
- [ ] test_integration_server_config.cpp
- [ ] test_integration_sensor_data.cpp
- [ ] test_integration_full_stack.cpp

---

**Progress: 5 / 63 tests implemented (7.9%)**

**Priority P0 (Critical)**: 24 remaining
**Priority P1 (High)**: 22 remaining  
**Priority P2 (Medium)**: 12 remaining

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

