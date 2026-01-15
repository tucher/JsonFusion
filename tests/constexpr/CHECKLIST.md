# Constexpr Test Implementation Checklist

Quick reference for tracking test implementation progress.

## Floating-Point Note

The in-house floating-point parser is constexpr-compatible. Do not use `JSONFUSION_FP_BACKEND=1` in constexpr tests.

## io/ (1 file) ✅ COMPLETE
- [x] test_io_iterators.cpp - Byte-by-byte streaming with custom iterators

## concepts/ (8 files) ✅ COMPLETE
- [x] test_json_parsable_value.cpp
- [x] test_json_serializable_value.cpp
- [x] test_consuming_streamer_like.cpp
- [x] test_producing_streamer_like.cpp
- [x] test_map_stdlib_interface.cpp
- [x] test_structural_detection.cpp
- [x] test_annotations_propagation.cpp
- [x] test_transformer_concepts.cpp

## primitives/ (11 files) ✅ COMPLETE
- [x] test_parse_int.cpp
- [x] test_parse_bool.cpp
- [x] test_parse_bool_edge_cases.cpp
- [x] test_parse_char_array.cpp
- [x] test_parse_float.cpp
- [x] test_parse_integers_signed.cpp
- [x] test_parse_integers_unsigned.cpp
- [x] test_parse_integers_overflow.cpp
- [x] test_parse_integers_edge_cases.cpp
- [x] test_parse_strings_escaping.cpp
- [x] test_parse_strings_edge_cases.cpp

## composite/ (15 files) ✅ COMPLETE
- [x] test_parse_nested_flat.cpp
- [x] test_parse_nested_complex.cpp
- [x] test_parse_array_primitives.cpp
- [x] test_parse_array_nested.cpp
- [x] test_parse_array_edge_cases.cpp
- [x] test_parse_optional_primitives.cpp
- [x] test_parse_optional_nested.cpp
- [x] test_parse_optional_edge_cases.cpp
- [x] test_parse_unique_ptr_primitives.cpp
- [x] test_parse_unique_ptr_nested.cpp
- [x] test_parse_string.cpp
- [x] test_parse_vector_primitives.cpp
- [x] test_parse_vector_nested.cpp
- [x] test_cpp_zero_sized.cpp
- [x] test_cpp_default_values.cpp

## json_spec/ (7 files) ✅ COMPLETE
- [x] test_json_whitespace.cpp - Whitespace handling (RFC 8259)
- [x] test_json_field_order.cpp - Field ordering independence
- [x] test_json_null.cpp - Null handling (std::optional, std::unique_ptr)
- [x] test_json_syntax_valid.cpp - Valid JSON syntax (empty keys, deep nesting, large arrays)
- [x] test_json_syntax_invalid.cpp - Invalid JSON syntax error detection
- [x] test_json_strings_unicode.cpp - Unicode escapes, surrogate pairs, RFC 8259 compliance
- [x] test_json_numbers_format.cpp - RFC 8259 number format compliance

## validation/ (15 files) ✅ COMPLETE
- [x] test_map_validators.cpp - All map validators (45 tests)
  - `min_properties`, `max_properties`
  - `min_key_length`, `max_key_length`
  - `required_keys`, `allowed_keys`, `forbidden_keys`
- [x] test_string_enum.cpp - String enum validation (17 tests)
  - `enum_values<...>` with incremental validation
- [x] test_validation_constant.cpp - `constant<>` (bool, integer, float, double)
- [x] test_validation_range_int.cpp - `range<>` for integers
- [x] test_validation_range_float.cpp - `range<>` for float and double
- [x] test_validation_range_signed.cpp - `range<>` for all signed int types
- [x] test_validation_range_unsigned.cpp - `range<>` for all unsigned int types
- [x] test_validation_string_length.cpp - `min_length<>`, `max_length<>`
- [x] test_validation_array_items.cpp - `min_items<>`, `max_items<>`
- [x] test_validation_struct_fields_presence.cpp - `not_required<>`/`required<>` (object-level)
- [x] test_validation_allow_excess_fields.cpp - `allow_excess_fields` (struct-level)
- [x] test_validation_combined_string.cpp - Multiple string validators
- [x] test_validation_combined_array.cpp - Multiple array validators
- [x] test_validation_combined_map.cpp - Multiple map validators
- [x] test_fn_validator.cpp - Custom function validators (`fn_validator<Event, Lambda>`)

## serialization/ (9 files) ✅ COMPLETE
- [x] test_serialize_int.cpp
- [x] test_serialize_bool.cpp
- [x] test_serialize_float.cpp
- [x] test_serialize_integers_all_types.cpp
- [x] test_serialize_strings.cpp
- [x] test_float_decimals.cpp
- [x] test_pretty_print.cpp - Pretty-printed JSON output (`JsonIteratorWriter<..., Pretty=true>`)
- [x] test_size_estimation.cpp - Compile-time size estimation (`EstimateMaxSerializedSize`)
- [x] test_skip_nulls.cpp - Skip null optional fields during serialization (`skip_nulls`)

## roundtrip/ (3 files) ✅ COMPLETE
- [x] test_roundtrip_primitives.cpp
- [x] test_roundtrip_nested.cpp
- [x] test_roundtrip_annotated.cpp

## streaming/ (6 files) ✅ COMPLETE
- [x] test_map_streaming.cpp - Map consumer/producer streamers
- [x] test_streamer_consumer_primitives.cpp - Array consumers for primitives
- [x] test_streamer_map_consumer.cpp - Map consumers with std::array<char,N> and std::string keys
- [x] test_streamer_producer_primitives.cpp - Array producers for primitives
- [x] test_streamer_map_producer.cpp - Map producers with std::array<char,N> and std::string keys
- [x] test_string_streaming.cpp - String streaming

## errors/ (7 files) ✅ COMPLETE
- [x] test_error_demo.cpp
- [x] test_error_json_path_primitives.cpp
- [x] test_error_json_path_arrays.cpp
- [x] test_error_json_path_maps.cpp
- [x] test_error_json_path_mixed.cpp
- [x] test_error_json_path_depth_calculation.cpp
- [x] test_error_path_annotations.cpp

## limits/ (4 files) ✅ COMPLETE
- [x] test_limits_nesting_depth.cpp - Deep nesting (10 levels, arrays, optionals, mixed)
- [x] test_limits_large_arrays.cpp - Large arrays (100 ints, 50 bools, 20 strings, 10x10 matrix, 50 in struct)
- [x] test_limits_many_fields.cpp - Many fields (50 fields, 30 mixed, nested, array fields)
- [x] test_limits_many_map_keys.cpp - Many map keys (30 allowed, 20 required, 30 forbidden, combined, binary search threshold)

## fp/ (5 files) ✅ COMPLETE
- [x] test_fp_boundary_values.cpp - Powers of 2, powers of 10, scientific notation
- [x] test_fp_difficult_cases.cpp - David Gay suite samples, rounding boundaries
- [x] test_fp_subnormals.cpp - Subnormal numbers, gradual underflow
- [x] test_fp_exponent_extremes.cpp - DBL_MAX/DBL_MIN edge cases
- [x] test_fp_serialization_roundtrip.cpp - FP serialization roundtrips

## wire_sink/ (4 files) ✅ COMPLETE
- [x] test_wire_sink_basic.cpp - Basic write/read, size tracking
- [x] test_wire_sink_concept_check.cpp - Concept verification
- [x] test_wire_sink_json_integration.cpp - JSON parser integration
- [x] test_wire_sink_static_schema.cpp - Static schema definition

## cbor/ (4 files) ✅ COMPLETE
- [x] test_cbor_primitives.cpp - CBOR serialization/deserialization
- [x] test_cbor_wiresink_roundtrip.cpp - CBOR with WireSink
- [x] test_cbor_int_key.cpp - Integer keys with `int_key<N>` annotation (CBOR-native)
- [x] test_cbor_indexes_as_keys.cpp - Automatic index keys with `indexes_as_keys` (CBOR-native)

## options/ (1 file) ✅ COMPLETE
- [x] test_annotated_skip_json.cpp - Fast-skip annotation

## transformers/ (2 files) ✅ COMPLETE
- [x] test_lambda_transformer.cpp - Custom transformation lambdas
- [x] test_variant_oneof.cpp - `VariantOneOf` transformer for `std::variant`

## json_schema/ (1 file) ✅ COMPLETE
- [x] test_json_schema_combined.cpp - Combined schema validators

## interaction/ (1 file) ✅ COMPLETE
- [x] test_c_interop.cpp - C language interoperability

---

**Total: 104 test files across 17 categories**

All categories complete.

