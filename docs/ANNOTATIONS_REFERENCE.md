## JsonFusion Types Annotation Reference

### Annotation Syntax

```cpp
Annotated<T, Validator1, Validator2, Option1, ...> field;
A<T, Validator1, Validator2, Option1, ...> field; //alias
```

### Validators (Type-Specific Constraints)

#### Number Validators
```cpp
range<Min, Max>              // Value must be in [Min, Max]
constant<Value>              // Value must equal Value (currently only for bool)
```

#### String Validators
```cpp
min_length<N>                // String must have at least N characters
max_length<N>                // String must have at most N characters (streaming)
enum_values<"val1", ...>     // String must be one of the listed values (streaming)
```

#### Array Validators
```cpp
min_items<N>                 // Array must have at least N elements
max_items<N>                 // Array must have at most N elements (streaming)
```

#### Struct Validators
```cpp
not_required<"field1", ...>  // Mark specific fields as optional - allows field to be absent from JSON (struct-level)
required<"field1", ...>  // Mark specific fields as required - forces field presence in JSON (struct-level)
forbidden<"field1", ...> // Mark specific fields as forbidden: error on presense
allow_excess_fields<depth=64>  // Allow unknown JSON fields (don't reject and silently skip up to `depth` values)
```

**Important**: `std::optional<T>`/`std::unique_ptr<T>` allows `null` values but the field presence is controlled by `not_required`/`required`

#### Map Validators
```cpp
// Entry count
min_properties<N>            // Map must have at least N entries
max_properties<N>            // Map must have at most N entries (streaming)

// Key constraints
min_key_length<N>            // All keys must have at least N characters
max_key_length<N>            // All keys must have at most N characters (streaming)

// Key whitelist/blacklist
required_keys<"k1", ...>     // These keys MUST be present
allowed_keys<"k1", ...>      // ONLY these keys are allowed (streaming)
forbidden_keys<"k1", ...>    // These keys are FORBIDDEN (streaming)
```

#### Generic Custom Validators

JsonFusion's validation system is event-driven. You can attach custom validation logic to parsing events using `fn_validator<Event, Callable>`.

**Syntax:**
```cpp
fn_validator<EventTag, Callable>
```

where:
- `EventTag` — A parsing event tag from `JsonFusion::validators_detail::parsing_events_tags`
- `Callable` — A stateless lambda or free function with signature matching the event

**Available Events & Signatures:**

The validator function can use simplified signatures (without `ValidationCtx`). The framework supports multiple signature variants but these are the most user-friendly:

| Event Tag | Triggers When | User Signature (simplified) | Example |
|-----------|--------------|------------------------------|---------|
| `bool_parsing_finished` | After parsing bool | `bool fn(const bool& value)` | `[](bool v) { return v == true; }` |
| `number_parsing_finished` | After parsing number | `bool fn(const T& value)` | `[](int v) { return v % 10 == 0; }` |
| `string_parsing_finished` | After parsing string | `bool fn(const Storage&, const std::string_view& value)` | `[](const auto&, const auto& s) { return s.size() > 5; }` |
| `array_item_parsed` | After each array item | `bool fn(const Container&, std::size_t count)` | `[](const auto&, size_t n) { return n <= 100; }` |
| `array_parsing_finished` | After complete array | `bool fn(const Container&, std::size_t count)` | `[](const auto& arr, size_t n) { return n >= 1; }` |
| `object_parsing_finished` | After complete struct | `bool fn(const Struct&, const auto& seen, const auto&)` | `[](const auto& obj, auto, auto) { return obj.isValid(); }` |
| `destructured_object_parsing_finished` | After `as_array` struct | `bool fn(const Struct&)` | `[](const Point& p) { return p.x >= 0; }` |
| `map_key_finished` | After each map key | `bool fn(const Map&, const std::string_view& key)` | `[](const auto&, const auto& k) { return k.size() <= 50; }` |
| `map_value_parsed` | After each map value | `bool fn(const Map&, std::size_t count)` | `[](const auto&, size_t n) { return n <= 1000; }` |
| `map_entry_parsed` | After key+value pair | `bool fn(const Map&, std::size_t count)` | `[](const auto&, size_t n) { return n <= 1000; }` |
| `map_parsing_finished` | After complete map | `bool fn(const Map&, std::size_t count)` | `[](const auto&, size_t n) { return n >= 1; }` |

**Note**: The first parameter is always the storage object being validated. For many validators, you can ignore it using `const auto&` and focus on the phase-specific parameters (like `std::string_view` for strings, `std::size_t count` for arrays/maps).


### Options (Metadata & Behavior Control)

#### Field-Level Options
```cpp
key<"field_name">              // Override JSON key name (use "field_name" instead of C++ field name)
exclude                     // Exclude field from JSON serialization/deserialization
description<"text">          // Documentation metadata for schema generation
skip<depth=64>          // Fast-skip this value without underlying JSON, with handling up to `depth` levels
wire_sink<depth=64, max_length=1<<16>          // Capture RAW JSON into underlying string-like object, with fixed maximum nesting level and limited length. Whitespaces are removed. Validates json overall correctness, does not check anything else.
```

#### Struct-Level Options
```cpp
as_array                     // Serialize/parse struct as JSON array instead of object: [x, y, z] <-> struct{float x, y, z;}
```



