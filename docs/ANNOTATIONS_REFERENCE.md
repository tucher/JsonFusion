## JsonFusion Types Annotation Reference

### Annotation Syntax

```cpp
Annotated<T, Validator1, Validator2, Option1, ...> field;
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
not_required<"field1", ...>  // Mark specific fields as optional (struct-level)
```

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

### Options (Metadata & Behavior Control)

#### Field-Level Options
```cpp
key<"jsonName">              // Override JSON key name (use "jsonName" instead of C++ field name)
not_json                     // Exclude field from JSON serialization/deserialization
description<"text">          // Documentation metadata for schema generation
skip_json                    // Fast-skip this value without underlying JSON
skip_materializing           // Where applicable, do as less C++-side work as possible. For doubles: skip string->float conversion. Does not affect JSON validness checking
```

#### Struct-Level Options
```cpp
allow_excess_fields          // Allow unknown JSON fields (don't reject and silently skip)
as_array                     // Serialize/parse struct as JSON array instead of object: [x, y, z] <-> struct{float x, y, z;}
```



