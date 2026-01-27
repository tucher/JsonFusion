# Error Handling Reference

JsonFusion provides layered error handling: simple bool checks for quick validation, or detailed introspection for diagnostic tooling.

## Quick Start: Bool Check + Formatted String

```cpp
Config cfg;
auto result = Parse(cfg, json_data);
if (!result) {
    std::cerr << ParseResultToString<Config>(result, json_data.begin(), json_data.end()) 
              << std::endl;
    return;
}
// Use cfg...
```

**Output example:**
```
When parsing $.bool_const_f, validator #0 (constant) error: 'wrong_constant_value': 
'..."bool_const_f": true,😖"string_c": "I am s...'
```

The formatted string includes:
- JSON path (semantic location: `$.field.nested[3].item`)
- Error type (parsing error or validator name + validatation error)
- Visual context (surrounding JSON with 😖 marker at error position)

## Advanced: ParseResult API

For diagnostic tools, logging, or custom error handling, inspect `ParseResult` directly.

### 1. Input Iterator Position

```cpp
auto result = Parse(cfg, json_data);
if (!result) {
    auto error_pos = result.pos();  // Input iterator at error location
    // error_pos points to the character where parsing failed
}
```

**Use case:** Highlighting errors in editors, restarting parsing from a known position.

### 2. Byte Offset

```cpp
auto result = Parse(cfg, json_data);
if (!result) {
    auto offset = result.offset();  // Byte offset from start
    // Works with iterators supporting operator-
}
```

**Use case:** Logging, error reporting systems expecting numeric offsets.

### 3. Error Code

```cpp
auto result = Parse(cfg, json_data);
if (!result) {
    ParseError err = result.error();
    
    // Parse errors (structural JSON issues):
    // - UNEXPECTED_END_OF_INPUT
    // - ILLFORMED_BOOL, ILLFORMED_NUMBER, ILLFORMED_STRING
    // - UNEXPECTED_TOKEN, MISSING_COMMA, MISSING_COLON
    // - NULL_IN_NON_OPTIONAL
    // - etc.
    
    if (err == ParseError::SCHEMA_VALIDATION_ERROR) {
        // Validation error, see validationErrors() below
    }
}
```

**Use case:** Programmatic error handling, telemetry, error categorization.

### 4. JSON Path

```cpp
auto result = Parse(cfg, json_data);
if (!result) {
    const auto& path = result.errorPath();
    
    // Examine path structure:
    for (size_t i = 0; i < path.currentLength; ++i) {
        if (path.storage[i].array_index != std::numeric_limits<size_t>::max()) {
            // Array index
            std::cout << "[" << path.storage[i].array_index << "]";
        } else {
            // Object field or map key
            std::cout << "." << path.storage[i].field_name;
        }
    }
}
```

**Path structure:**
- `currentLength`: Number of path segments
- `storage[i].array_index`: Array index or `max()` for non-array
- `storage[i].field_name`: Field name (`std::string_view`)
- `storage[i].is_static`: Static (compile-time) vs dynamic (map key)

**Use case:** Navigate to error location programmatically, build custom error messages.

### 5. Validation Errors

```cpp
auto result = Parse(cfg, json_data);
if (!result && result.error() == ParseError::SCHEMA_VALIDATION_ERROR) {
    auto val_result = result.validationErrors();
    
    // Which validator failed:
    size_t validator_idx = val_result.validator_index();
    
    // What went wrong:
    SchemaError schema_err = val_result.error();
    
    // Possible SchemaError values:
    // - number_out_of_range          (range<> violated)
    // - string_length_out_of_range   (min_length<>/max_length<>)
    // - array_items_count_out_of_range (min_items<>/max_items<>)
    // - map_properties_count_out_of_range (min_properties<>/max_properties<>)
    // - map_key_length_out_of_range  (min_key_length<>/max_key_length<>)
    // - map_key_not_allowed          (allowed_keys<>)
    // - map_key_forbidden            (forbidden_keys<>)
    // - map_missing_required_key     (required_keys<>)
    // - wrong_constant_value         (constant<>, string_constant<>)
    // - wrong_enum_value             (enum_values<>)
    // - missing_required_fields      (required/not_required)
}
```

**Use case:** Distinguish between different validation failures, report which constraint was violated.

### 6. Inspect Annotations at Error Location

```cpp
auto result = Parse(cfg, json_data);
if (!result) {
    const auto& path = result.errorPath();
    
    // Visit field options at error location:
    path.template visit_options<Config>([](auto opts) {
        using Opts = decltype(opts);
        
        // Check what annotations are present:
        if constexpr (Opts::template has_option<validators::range_tag>()) {
            using RangeOpt = typename Opts::template get_option<validators::range_tag>;
            std::cout << "Expected range: [" 
                      << RangeOpt::min_v << ", " << RangeOpt::max_v << "]\n";
        }
        
        // Access validator descriptions:
        if constexpr (!std::is_same_v<Opts, options::detail::no_options>) {
            std::string_view validator_name = 
                error_formatting_detail::get_opt_string_helper<Opts>::get(
                    result.validationErrors().validator_index()
                );
            std::cout << "Validator: " << validator_name << "\n";
        }
    });
}
```

**Use case:** Extract expected values from annotations, build detailed validation reports.

### 7. Visit Object at Error Location

```cpp
auto result = Parse(cfg, json_data);
// Note: Partial parse may have left object in inconsistent state
// Use with caution

const auto& path = result.errorPath();

// Visit the field that caused the error:
path.visit(cfg, [](auto& field_value, auto opts) {
    using T = std::decay_t<decltype(field_value)>;
    
    std::cout << "Error at field of type: " << typeid(T).name() << "\n";
    
    // Apply default value, log current state, etc.
    if constexpr (std::is_same_v<T, int>) {
        field_value = 0;  // Set to default
    }
});
```

**Use case:** Apply defaults on validation failures, inspect partial parse state for debugging.

## Error Handling Patterns

### Pattern 1: Fast Fail

```cpp
if (!Parse(cfg, json)) return std::nullopt;
```

### Pattern 2: Log and Continue

```cpp
auto result = Parse(cfg, json);
if (!result) {
    logger.error(ParseResultToString<Config>(result, json.begin(), json.end()));
    return default_config();
}
```

### Pattern 3: Detailed Diagnostics

```cpp
auto result = Parse(cfg, json);
if (!result) {
    DiagnosticReport report;
    report.offset = result.offset();
    report.json_path = format_path(result.errorPath());
    report.error_type = result.error();
    
    if (result.error() == ParseError::SCHEMA_VALIDATION_ERROR) {
        report.validator_index = result.validationErrors().validator_index();
        report.schema_error = result.validationErrors().error();
        
        // Extract expected values from annotations
        result.errorPath().template visit_options<Config>([&](auto opts) {
            report.constraints = extract_constraints(opts);
        });
    }
    
    return report;
}
```

### Pattern 4: Apply Defaults on Validation Failure

```cpp
auto result = Parse(cfg, json);
if (!result && result.error() == ParseError::SCHEMA_VALIDATION_ERROR) {
    // Validation failed, but parse succeeded structurally
    // Apply defaults to invalid fields:
    result.errorPath().visit(cfg, [](auto& field, auto opts) {
        apply_default(field, opts);
    });
}
```

## Implementation Notes

### ParseResult Template Parameters

```cpp
template<CharInputIterator InpIter, std::size_t MaxSchemaDepth, bool HasMaps>
struct ParseResult;
```

- `InpIter`: Input iterator type (often `const char*` or `std::string_view::iterator`)
- `MaxSchemaDepth`: Compile-time calculated from `calc_type_depth<T>()`
- `HasMaps`: Compile-time flag from `schema_analyzis::has_maps<T>()`

### Zero-Overhead Design

- Path tracking on **success**: Only counter increments + `std::string_view` assignments
- Path storage: Stack-allocated `std::array<PathElement, MaxSchemaDepth>`
- Map key buffers: Only allocated when `HasMaps == true`
- No heap allocation unless cyclic recursive types detected AND it is allowed by global JSONFUSION_ALLOW_DYNAMIC_ERROR_STACK macro

### Constexpr Compatibility

All error handling mechanisms work in `constexpr` contexts:

```cpp
static_assert([]() constexpr {
    Config cfg;
    auto result = Parse(cfg, R"({"field": "invalid"})");
    return !result && result.error() == ParseError::SCHEMA_VALIDATION_ERROR;
}());
```

## Summary

**For most cases:** Use `ParseResultToString<T>()` for human-readable diagnostics.

**For tooling:** Access `ParseResult` directly:
- `pos()` / `offset()` → error location
- `error()` → error code
- `errorPath()` → semantic path
- `validationErrors()` → constraint details
- `visit_options()` / `visit()` → annotation/value inspection

All mechanisms are **type-driven**: information comes from compile-time schema analysis, not runtime configuration.

