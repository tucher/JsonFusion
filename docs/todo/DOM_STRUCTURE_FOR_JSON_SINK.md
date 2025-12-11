# Internal DOM Structure for `json_sink`

## Current Situation

`json_sink` currently captures JSON subtrees as `std::string`:
```cpp
using wire_type = A<std::string, json_sink<>>;
```

**Problems:**
- Re-parsing requires re-tokenization (lexing overhead)
- String format is verbose (spaces, quotes, escapes)
- Every re-parse in transformers like `VariantOneOf` pays lexing cost N times

## Proposed Solution

Add an internal DOM structure (`JsonNode`) and corresponding `DomReader`:

```cpp
// Minimal DOM - just the 6 JSON types
struct JsonNode {
    enum class Type { Null, Bool, Number, String, Array, Object };
    Type type;
    // ... value storage (union, variant, or tagged union)
};

// Reader that reads from DOM
class DomReader /* : ReaderLike */ {
    // Reads from JsonNode tree instead of char stream
};

// json_sink can target DOM or string
template<class Target = std::string>
struct json_sink { /* ... */ };
```

## Benefits

1. **Faster re-parsing** - No re-tokenization, tokens already parsed
2. **More compact** - Binary representation vs text format
3. **Enables DOM parsing** - Parse directly from `JsonNode` when needed
4. **Better for schema transformers** - `VariantOneOf` becomes much more efficient

## Architecture Fit

This is **Layer 1 infrastructure**, not Layer 3:
- Just another Reader implementation (like yyjson Reader)
- Still models only the 6 JSON types
- No special cases - `DomReader` implements `ReaderLike`, that's it
- Opt-in - only used when you need `json_sink<JsonNode>`
- Composable with all existing features

## Use Cases

### Efficient VariantOneOf
```cpp
struct VariantOneOf {
    using wire_type = A<JsonNode, json_sink<JsonNode>>;
    
    bool transform_from(const JsonNode& node) {
        // Parse from DOM - no re-tokenization
        DomReader reader(node);
        // Try alternatives...
    }
};
```

### Hybrid Parsing
```cpp
struct Message {
    std::string type;
    A<JsonNode, json_sink<JsonNode>> payload;
};

// Dispatch based on type field
if (msg.type == "event") {
    Event e;
    DomReader r(msg.payload);
    ParseWithReader(e, r);
}
```

### DOM Mode
```cpp
// Parse to DOM first
JsonNode dom;
Parse(dom, json_string);

// Later, parse DOM into types
MyStruct obj;
DomReader reader(dom);
ParseWithReader(obj, reader);
```

## Implementation Notes

**Memory management:**
- Use `std::string_view` for strings when possible (zero-copy from original buffer)
- Fall back to owned strings when needed (escaped chars, lifetime issues)
- Consider small-buffer optimization for numbers

**Start simple:**
```cpp
struct JsonNode {
    std::variant<
        std::monostate,              // null
        bool,
        double,
        std::string,                 // or string_view with lifetime management
        std::vector<JsonNode>,
        std::vector<std::pair<std::string, JsonNode>>
    > value;
};
```

**Optimize later if needed:**
- Tagged union for better cache locality
- Arena allocation for tree
- Small value optimization
- Custom string storage strategy

## Priority

Medium - Nice optimization, especially valuable for:
- `VariantOneOf` and similar schema transformers
- Applications that need occasional DOM-style access
- Embedded targets where string re-parsing overhead matters

Not blocking core functionality, but improves Layer 3 building blocks.
