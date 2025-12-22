# JSON Type Mapping in JsonFusion

## Core Principle

JsonFusion maps JSON types to C++ types with a two-layer schema system:

1. **Implicit Schema (Type System)**: C++ type defines fundamental constraints (capacity, uniformity, nullability)
2. **Explicit Schema (Annotations)**: Optional validators add constraints **above** and **after** type system checks

**Validation Order**: Type system checks → Annotation validators

---

## ⚡ CRITICAL: Streaming Validation (Single-Pass)

**ALL validation happens DURING parsing, not after.** JsonFusion is a **streaming parser** that validates **AS SOON AS POSSIBLE** with **early rejection**.
Because there are some checks that logically happen “at the end of a thing”: min_length<> (after string), min_items<> (after array), object_parsing_finished for required<>/not_required<>. But they are still single-pass: they happen as soon as the parser knows the boundary is closed.

This is the **fundamental reason** why JsonFusion can be:
- ✅ **`constexpr`**: No dynamic allocation required
- ✅ **Zero-allocation**: No buffering of entire input
- ✅ **Fast**: Reject immediately on first violation

**Examples:**
- `max_length<10>` on string: Rejected at **11th character**, not after parsing entire string
- `max_items<5>` on array: Rejected at **6th element**, not after parsing entire array
- `max_properties<100>` on map: Rejected at **101st entry**, not after parsing entire map
- Capacity overflow: Rejected **immediately** when limit is reached

**All validators below operate in single-pass streaming mode unless explicitly noted otherwise.**

---

## 1. `null` → `std::optional<T>`/`std::unique_ptr<T>`

### JSON Standard
```json
{"value": null}
{"value": 42}
```

### C++ Mapping
```cpp
struct Data {
    std::optional<int> value;
};
```

### Implicit Schema (Type System)
- Non-optional field: **null is rejected**
- Optional field: **null is accepted** (but field must still be present in JSON)

```cpp
struct Required {
    int value;  // null → error: NULL_IN_NON_OPTIONAL
};

struct Optional {
    std::optional<int> value;  // null → accepted, stored as std::nullopt
};
```

### ⚠️ Important: Nullable vs Absent

**`std::optional<T>`/`std::unique_ptr<T>` means nullable, NOT absent:**
- ✅ Field present with `null` value → accepted (stored as `std::nullopt`/`nullptr`)
- ✅ Field present with value → accepted
- ❌ Field absent from JSON → **error** (field is still required, if `required<>`/`not_required<>` say so)


**Design Rationale:**
- `std::optional<T>`/`std::unique_ptr<T>` handles **nullability** (field-level concern: "can this value be null?")
- `required<>`/`not_required<>` handles **presence** (object-level concern: "must this field be in JSON?")
- This separation allows precise control: a field can be nullable but required, or non-nullable but optional

### Design Rule
**All nullability is expressed via `std::optional<T>`/`std::unique_ptr<T>`.** There is no "unknown type behind null" - the type is always known at compile time.

---

## 2. `number` → C++ Numeric Types

### JSON Standard
```json
{"value": 42}
{"value": -3.14}
{"value": 1.5e10}
```

### C++ Mapping
```cpp
int value;
double value;
int8_t value;
uint64_t value;
```

### Implicit Schema (Type System)
**Always enforced:**
1. **Capacity check**: Value must fit in C++ type's range
2. **No implicit casting**: Integer JSON values only go into integer C++ types
3. **Float rejection for integers**: `42.5` rejected for `int` storage

```cpp
struct Config {
    int8_t value;  // Accepts: -128 to 127
};

Parse(config, R"({"value": 128})");    // Error: NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE
Parse(config, R"({"value": 42.5})");   // Error: FLOAT_VALUE_IN_INTEGER_STORAGE
Parse(config, R"({"value": "42"})");   // Error: ILLFORMED_NUMBER (no implicit casting)
```

**Overflow behavior:**
- Boundary: `INT8_MAX + 1` → error
- Large: `99999999999999999999` → error
- Absurd: `999999999999999999999999999999` → error (gracefully handled)

### Explicit Schema (Annotations)

**Range validation:**
```cpp
Annotated<int, range<0, 100>> percentage;
Annotated<double, range<-273.15, 1000.0>> temperature;
```

**Constant values:**
```cpp
Annotated<bool, constant<true>> must_be_true;
Annotated<int, constant<42>> magic_number;  // TODO: not fully implemented yet
```

**Validation order (streaming):**
1. Parse number from JSON **with capacity checking during parsing** (implicit):
   - As digits are parsed, value is built up
   - **Immediately** reject if value exceeds type capacity (e.g., `128` for `int8_t`)
   - Early termination on overflow
   - float numbers are parsed to a buffer of sufficient size, then converted to `double`, then checked for starage range
2. After successful parse: **Immediately** check `range<>` constraint if present (explicit)
3. After successful parse: **Immediately** check `constant<>` constraint if present (explicit)

All checks happen **during or immediately after** parsing. Rejection is immediate.

---

## 3. `string` → C++ String Containers

### JSON Standard
```json
{"name": "hello"}
{"text": "a\nb\tc"}
```

### C++ Mapping
```cpp
std::string name;                // Dynamic
std::array<char, 64> name;       // Fixed-size
std::vector<char> name;          // Dynamic
```

### Implicit Schema (Type System)
**Fixed-size containers:**
- **Max length**: `size - 1` (null terminator required)
- Overflow → `FIXED_SIZE_CONTAINER_OVERFLOW`

```cpp
struct Data {
    std::array<char, 5> text;  // Max 4 characters + null
};

Parse(data, R"({"text": "abc"})");    // OK: 3 chars
Parse(data, R"({"text": "abcd"})");   // OK: 4 chars
Parse(data, R"({"text": "abcde"})");  // Error: FIXED_SIZE_CONTAINER_OVERFLOW
```

**Dynamic containers:**
- No implicit size limit

### Explicit Schema (Annotations)

**Length constraints:**
```cpp
Annotated<std::string, min_length<1>> non_empty;
Annotated<std::string, max_length<100>> limited;
Annotated<std::string, min_length<3>, max_length<32>> bounded;

Annotated<std::array<char, 64>, min_length<1>> fixed_non_empty;
```

**Enum validation (whitelist of allowed values):**
```cpp
Annotated<std::string, enum_values<"red", "green", "blue">> color;
Annotated<std::string, enum_values<"GET", "POST", "PUT", "DELETE">> http_method;

// JSON: {"color": "red"}     → ✓ accepted
// JSON: {"color": "yellow"}  → ✗ rejected (not in enum)
```

**Note:** `enum_values<>` uses **incremental character-by-character validation** with early rejection - if a prefix doesn't match any allowed value, parsing fails immediately.

**Validation order (streaming, character-by-character):**

**Fixed-size:**
1. For each character parsed:
   - **Immediately** check capacity (implicit: `size - 1`)
   - **Immediately** check `max_length<>` if present (explicit)
   - **Immediately** check `enum_values<>` incremental match if present (explicit)
   - Reject at **first violation** (early termination)
2. After parsing complete: check `min_length<>` if present (explicit)
3. After parsing complete: check `enum_values<>` final match if present (explicit)

**Dynamic:**
1. For each character parsed:
   - Append to dynamic container
   - **Immediately** check `max_length<>` if present (explicit)
   - **Immediately** check `enum_values<>` incremental match if present (explicit)
   - Reject at **first violation** (early termination)
2. After parsing complete: check `min_length<>` if present (explicit)
3. After parsing complete: check `enum_values<>` final match if present (explicit)

**Examples:** 
- `max_length<10>` rejects at the **11th character**, not after parsing the entire string
- `enum_values<"red", "green">` parsing "yellow" rejects at character 'y' (doesn't match any prefix)

---

## 4. `array` → C++ Uniform Arrays

### JSON Standard
```json
{"items": [1, 2, 3]}
{"values": []}
```

### C++ Mapping
```cpp
std::vector<int> items;
std::array<int, 10> items;

// Or Array-Streamers (JsonFusion's strongly-typed streaming API)
ArrayConsumer items;  // For parsing
static_assert(JsonFusion::ConsumingStreamerLike<ArrayConsumer>);

ArrayProducer items;  // For serialization
static_assert(JsonFusion::ProducingStreamerLike<ArrayProducer>);
```

**Note:** Array-Streamers are JsonFusion's streaming API that can be used as direct replacements for array-like containers anywhere in the schema, providing custom consumption/production logic.

### Design Constraint
**Uniform arrays only.** All elements must have the same C++ type.

```cpp
std::vector<int> numbers;  // [1, 2, 3] ✓
// JSON [1, "two", 3] → Error: type mismatch at element 1
```

### Implicit Schema (Type System)
**Element type:**
- Defined by `value_type` of container
- Each element validated according to its type's schema

**Fixed-size containers:**
- **Max length**: `size`
- Overflow → `FIXED_SIZE_CONTAINER_OVERFLOW`

```cpp
struct Data {
    std::array<int, 3> values;  // Exactly 3 elements max
};

Parse(data, R"({"values": [1, 2]})");     // OK: 2 elements
Parse(data, R"({"values": [1, 2, 3]})");  // OK: 3 elements
Parse(data, R"({"values": [1, 2, 3, 4]})");  // Error: FIXED_SIZE_CONTAINER_OVERFLOW
```

**Dynamic containers:**
- No implicit size limit

### Explicit Schema (Annotations)
```cpp
Annotated<std::vector<int>, min_items<1>> non_empty;
Annotated<std::vector<int>, max_items<100>> limited;
Annotated<std::vector<int>, min_items<1>, max_items<10>> bounded;

Annotated<std::array<int, 100>, min_items<5>> at_least_five;
```

**Validation order (streaming, element-by-element):**

1. For each element parsed:
   - Parse element value (element type's implicit + explicit schema)
   - **Immediately** check capacity for fixed-size (implicit)
   - **Immediately** check `max_items<>` if present (explicit)
   - Reject at **first violation** (early termination)
2. After all elements: check `min_items<>` if present (explicit)

**Example:** `max_items<5>` rejects at the **6th element**, not after parsing the entire array.

---

## 5. `object` → Two Paradigms in C++

JSON objects map to **two fundamentally different** C++ patterns.
- **Structs**: compile-time field set, unknown keys rejected by default.
- **Maps**: runtime field set, unknown keys are the *normal* case.

### JSON Object Keys: Streaming String Validation

**In JSON stream, object keys are always strings:**
```json
{"keyName": value, "anotherKey": value}
```

**Key validation happens DURING parsing** (character-by-character), but the **semantics differ** between paradigms:

#### **Paradigm A (Fixed Structs)**: Compile-Time Key Matching
- Keys are **matched to field names** during parsing
- Unknown keys → **immediate rejection** (unless `allow_excess_fields`)
- No key length/pattern validation (field names are compile-time literals)

```cpp
struct Config { int port; std::string host; };
// JSON key "port" → matched to field `port`
// JSON key "unknown" → immediate error (before parsing value)
```

#### **Paradigm B (Dynamic Maps)**: Runtime Key Validation
- Keys are **runtime string values** stored in the map
- Key constraints validated **character-by-character** during parsing
- `min_key_length<>`, `max_key_length<>` applied **as key is parsed**
- Rejection at **first violation** (early termination)

```cpp
Annotated<std::map<std::string, int>, max_key_length<32>> data;
// JSON key parsed character-by-character
// At 33rd character → immediate rejection
```

**Both paradigms parse keys as strings in a single pass, but use them differently after validation.**

---

### Paradigm A: Fixed Structures (C++ Structs)

#### JSON Example
```json
{
  "name": "Alice",
  "age": 30,
  "email": "alice@example.com"
}
```

#### C++ Mapping
```cpp
struct Person {
    std::string name;
    int age;
    std::optional<std::string> email;
};
```

#### Implicit Schema (Type System)
**Always enforced:**
1. **Field names**: Compile-time fixed (or via `key<"name">` annotation)
2. **Field types**: Each field has its own type (can differ)
3. **Field count**: Fixed at compile time
4. **Required fields**: By default all struct fields are not required (may be absent in JSON), including `std::optional`/`std::unique_ptr<T>` fields. `std::optional`/`std::unique_ptr<T>` only allows `null` values, not controls presense/absence. To enforce presence, use struct-level `not_required<...>`/`required<...>` validators.
5. **Unknown fields**: Rejected by default

```cpp
struct Config {
    int port;
    std::string host;
};

Parse(config, R"({"port": 8080, "host": "localhost"})");  // OK
Parse(config, R"({"port": 8080})");                       // OK: "host" is not required by default 
Parse(config, R"({"port": 8080, "host": "localhost", "extra": 1})");  // Error: unknown field
```

**Per-field validation:**
Each field validated according to its own type's schema (recursive).

#### Explicit Schema (Annotations)

**Field-level validators:**
```cpp
struct Person {
    Annotated<std::string, min_length<1>> name;
    Annotated<int, range<0, 150>> age;
};
```

**Field-level options:**
```cpp
struct Config {
    // Custom JSON key name (override C++ field name)
    Annotated<int, key<"portNumber">> port;  // JSON: "portNumber", C++: port
    
    // Exclude from JSON serialization/deserialization
    Annotated<std::string, not_json> internal_cache;
    
    // Documentation metadata (for schema generation)
    Annotated<std::string, description<"User's email address">> email;
    
   
};
```

**Struct-level options:**
```cpp
// Allow unknown fields
Annotated<Config, allow_excess_fields> config;

// Explicit optional fields (struct-level annotation)
struct User {
    std::string nickname;  // 
    std::string email;     // Required
};
Annotated<User, not_required<"nickname">> user;  // Mark "nickname" as not required and all others required

// Serialize struct as JSON array instead of object (fields in order)
struct Point {
    double x;
    double y;
    double z;
};
Annotated<Point, as_array> point;  // JSON: [1.0, 2.0, 3.0] instead of {"x": 1.0, "y": 2.0, "z": 3.0}
```

**Validation order (streaming, field-by-field):**

1. For each field in JSON object:
   - **Parse key** (character-by-character as string)
   - **During/after key parsing**: match to struct field name
   - **Immediately** reject if unknown field (unless `allow_excess_fields`) - **before parsing value**
   - **Parse value** (field type's implicit + explicit schema, **streaming**)
2. After all fields: apply struct-level validators (explicit: `not_required<>`/`required<>`)

**Example:** Key `"extra"` is parsed, matched against field names, and rejected **immediately** - value parsing never starts.

**Note:** `min_properties`/`max_properties` do NOT apply - field count is fixed at compile time.

---

### Paradigm B: Dynamic Maps (std::map, Map-Streamers)

#### JSON Example
```json
{
  "alice": 100,
  "bob": 95,
  "charlie": 88
}
```

#### C++ Mapping
```cpp
std::map<std::string, int> scores;

// Or Map-Streamers (JsonFusion's strongly-typed streaming API):
MapConsumer scores;  // For parsing
static_assert(JsonFusion::ConsumingMapStreamerLike<MapConsumer>);

MapProducer scores;  // For serialization
static_assert(JsonFusion::ProducingMapStreamerLike<MapProducer>);
```

**Note:** Map-Streamers are JsonFusion's streaming API that can be used as direct replacements for map-like containers anywhere in the schema, providing custom consumption/production logic with full control over key-value storage.

#### Implicit Schema (Type System)
**Always enforced:**
1. **Key type**: Must be string-like
2. **Value type**: **Uniform** - all values have same type

```cpp
std::map<std::string, int> data;
// All values are int - no heterogeneous types possible
```

**Per-value validation:**
Each value validated according to value type's schema (recursive).

**Capacity (implementation-dependent):**
- `std::map`: Dynamic, no fixed limit
- Custom user-defined map-like containers: May have fixed capacity
  ```cpp
  MyCustomMap data;  // Implementation may define max entries
  // Overflow → FIXED_SIZE_CONTAINER_OVERFLOW (implicit constraint)
  ```

#### Explicit Schema (Annotations)

**Entry count constraints:**
```cpp
Annotated<std::map<std::string, int>,
          min_properties<1>,
          max_properties<100>> bounded_map;
```

**Key length constraints:**
```cpp
Annotated<std::map<std::string, int>,
          min_key_length<3>,
          max_key_length<32>> validated_keys;
```

**Key whitelist/blacklist:**
```cpp
// Require specific keys to be present
Annotated<std::map<std::string, int>,
          required_keys<"name", "age">> must_have_keys;

// Only allow specific keys (reject others)
Annotated<std::map<std::string, int>,
          allowed_keys<"name", "age", "email">> whitelist_keys;

// Forbid specific keys (reject if present)
Annotated<std::map<std::string, int>,
          forbidden_keys<"password", "secret">> blacklist_keys;
```

**Combined constraints:**
```cpp
Annotated<std::map<std::string, int>,
          min_properties<1>,
          max_properties<100>,
          min_key_length<2>,
          max_key_length<50>,
          required_keys<"id">,
          allowed_keys<"id", "name", "email", "age">> full_validation;
```

**Validation order (streaming, entry-by-entry):**

1. For each entry in JSON object:
   - **Parse key** (character-by-character):
     - **During parsing**: check `min_key_length<>` if present (explicit)
     - **During parsing**: check `max_key_length<>` if present (explicit)
     - **During parsing**: check `allowed_keys<>` incremental match if present (explicit)
     - **During parsing**: track `required_keys<>` match if present (explicit)
     - **During parsing**: track `forbidden_keys<>` match if present (explicit)
     - Reject at **first violation** (early termination)
   - **After key parsed**:
     - **Immediately** check `allowed_keys<>` final match if present (explicit)
     - **Immediately** check `forbidden_keys<>` final match if present (explicit)
   - **Parse value** (value type's implicit + explicit schema, **streaming**)
   - **After value parsed**: check `max_properties<>` if present (explicit)
   - Reject at **first violation** (early termination)
2. After all entries: 
   - Check `min_properties<>` if present (explicit)
   - Check `required_keys<>` completeness if present (explicit)

**Examples:**
- `max_properties<100>` rejects at the **101st entry**, not after parsing entire map
- `max_key_length<32>` rejects at the **33rd character of a key**, not after parsing the key
- `allowed_keys<"name", "age">` parsing key "email" rejects at character 'e' (doesn't match any allowed prefix)
- `required_keys<"name", "age">` fails at end if "name" or "age" was never seen
- `forbidden_keys<"password">` rejects immediately when key "password" is fully matched
- Capacity overflow rejects **immediately** when trying to add beyond capacity

**Note:** `properties` / `required` do NOT apply - keys are runtime values, not compile-time fields.


---

## Design Principles

1. **Streaming validation**: Single-pass parsing with immediate rejection (enables constexpr + zero-allocation)
2. **Type system first**: C++ types define fundamental constraints that always exist
3. **Annotations second**: Validators add constraints above type system checks
4. **No implicit casting**: Type mismatches are errors, not conversions
5. **Compile-time when possible**: Static type checking over runtime validation
6. **Explicit nullability**: `std::optional<T>` for nullable, never implicit
7. **Uniform containers**: Arrays and maps have single element/value type
8. **Two object paradigms**: Fixed structs vs dynamic maps - fundamentally different


## Conclusion

JsonFusion's type mapping leverages C++'s static type system to provide **implicit schemas** that are always enforced. Annotations provide **explicit schemas** for additional constraints. This two-layer approach ensures type safety while allowing flexible validation rules.

C++ types compatibility and explicitely defined additional validators don't wait for parsing to complete - they validate **as data arrives**, character by character, element by element, entry by entry.

