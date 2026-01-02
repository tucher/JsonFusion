# Custom Type Transformers

## Philosophy

JsonFusion does **not** include an endless list of built-in support for application-domain types (dates, UUIDs, currencies, etc.). Instead, it provides **generic building blocks** that let you define your own transformations.

This design keeps the library:
- **Minimal**: No bloat from types you don't use
- **Flexible**: You control exactly how transformations work
- **Composable**: Building blocks nest cleanly
- **Zero-overhead**: All transformations resolve at compile time

Think of it as providing a **Lego set**, not a **catalog**.

## Core Type System Hooks

At the foundation, JsonFusion recognizes two C++20 concepts that are **baked into the parser and serializer**:

### ParseTransformerLike Concept

```cpp
template<class T>
concept ParseTransformerLike = requires(T t, const typename T::wire_type& w) {
    typename T::wire_type;              // What appears in JSON
    { t.transform_from(w) } -> std::same_as<bool>;  // JSON → domain type
} && ParsableValue<typename T::wire_type>;
```

**How it works in the parser** (simplified):

```cpp
template <class Field>
    requires ParseTransformerLike<Field>
bool ParseValue(Field& field, Reader& reader, Context& ctx) {
    typename Field::wire_type wire;
    
    // 1. Parse JSON into wire_type (recursive - can be any ParsableValue!)
    bool ok = ParseValue(wire, reader, ctx);
    
    // 2. Transform wire → domain type
    if (ok) {
        if (!field.transform_from(wire)) {
            ctx.setError(ParseError::TRANSFORMER_ERROR);
            return false;
        }
    }
    return ok;
}
```

### SerializeTransformerLike Concept

```cpp
template<class T>
concept SerializeTransformerLike = requires(const T t, typename T::wire_type& w) {
    typename T::wire_type;              // What appears in JSON
    { t.transform_to(w) } -> std::same_as<bool>;    // Domain type → JSON
} && SerializableValue<typename T::wire_type>;
```

**How it works in the serializer** (simplified):

```cpp
template <class Field>
    requires SerializeTransformerLike<Field>
bool SerializeValue(const Field& field, Iterator& it, Context& ctx) {
    typename Field::wire_type wire;
    
    // 1. Transform domain type → wire
    if (!field.transform_to(wire)) {
        ctx.setError(SerializeError::TRANSFORMER_ERROR);
        return false;
    }
    
    // 2. Serialize wire_type to JSON (recursive!)
    return SerializeValue(wire, it, ctx);
}
```

**Key insight**: `wire_type` can be **anything** that's `ParsableValue` / `SerializableValue`:
- Primitives: `int`, `bool`, `string`
- Structs: `PersonWire { string first; string last; }`
- Arrays: `vector<ItemWire>`
- Maps: `map<string, Value>`
- Optionals: `optional<T>`
- **Even other transformers!**

The recursion makes the system fully composable with zero special cases.

## Building Blocks

JsonFusion provides ready-to-use transformer templates built on top of these core hooks:

### 1. Transformed<StoredT, WireT, FromFn, ToFn>

**Purpose**: Bidirectional 1-to-1 transformations.

```cpp
template<
    class StoredT,    // What you store in your C++ model
    class WireT,      // What appears in JSON
    auto FromFn,      // bool(StoredT&, const WireT&)  - parse
    auto ToFn         // bool(const StoredT&, WireT&)  - serialize
>
struct Transformed;
```

**Example - Date as String**:

```cpp
struct MyDate {
    int year, month, day;
    
    static bool from(MyDate& dst, const std::string& s) {
        // Parse "YYYY-MM-DD"
        if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
        // ... parse logic ...
        return true;
    }
    
    static bool to(const MyDate& src, std::string& out) {
        char buf[11];
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", 
                      src.year, src.month, src.day);
        out.assign(buf, 10);
        return true;
    }
};

using DateTransform = transformers::Transformed<
    MyDate,           // Store structured date
    std::string,      // Wire format is string
    &MyDate::from,    // Parse function
    &MyDate::to       // Serialize function
>;

struct Event {
    A<DateTransform, key<"date">> date;
    A<std::string,   key<"name">> name;
};

// JSON: {"date": "2023-12-31", "name": "New Year"}
// C++:  Event{ .date.value = MyDate{2023, 12, 31}, .name = "New Year" }
```

**Transparent access**: `Transformed` provides conversion operators, `operator->`, range forwarding, etc., making it mostly transparent:

```cpp
Event e;
Parse(e, R"({"date":"2023-12-31","name":"foo"})");

MyDate d = e.date.value;  // implicit conversion
std::cout << d.year;      // access via operator->
```

**Use cases**:
- Date/time parsing (ISO8601 string ↔ struct)
- UUID (string ↔ 128-bit value)
- Enum serialization (string ↔ enum)
- Base64/hex encoding
- Unit conversions (JSON: meters, C++: feet)
- Currency (JSON: cents as int, C++: dollars as double)
- Email/URL validation

### 2. ArrayReduceField<ElemWire, StoredT, ReduceFn>

**Purpose**: Parse JSON array, reduce/aggregate elements, store only the result.

```cpp
template<
    class ElemWire,   // Array element type in JSON
    class StoredT,    // Accumulated result type
    auto ReduceFn     // bool(StoredT& state, const ElemWire& elem)
>
struct ArrayReduceField;
```

**Example - Sum of Array**:

```cpp
auto sum_reducer = [](int& sum, int val) {
    sum += val;
    return true;
};

using SumField = transformers::ArrayReduceField<
    A<int, validators::range<0, 100>>,  // Each element validated
    int,                                 // Store sum
    sum_reducer                          // Reduction function
>;

struct Data {
    A<SumField, key<"values">> sum;
};

// JSON: {"values": [1, 2, 3, 4]}
// C++:  Data{ .sum.value = 10 }  - array not stored, only sum!
```

**Use cases**:
- Aggregations (sum, product, min, max, average)
- Counting/statistics
- Validation-only (parse and validate, don't store)
- Set operations (unique count, membership testing)
- Memory efficiency (process large arrays without materializing them)

### 3. MapReduceField<KeyWire, ValueWire, StoredT, ReduceFn> *(planned)*

**Purpose**: Parse JSON object/map, reduce entries, store only the result.

```cpp
template<
    class KeyWire,    // Map key type (usually string)
    class ValueWire,  // Map value type in JSON
    class StoredT,    // Accumulated result type
    auto ReduceFn     // bool(StoredT&, const KeyWire&, const ValueWire&)
>
struct MapReduceField;
```

**Example use cases**:
- Find max value across map entries
- Validate all map values meet criteria
- Count entries matching a condition
- Aggregate metrics from a map

### 4. WireSink as wire_type (Protocol-Agnostic Transformers)

**Purpose**: Capture raw protocol data without committing to a specific wire format.

When `wire_type` is `WireSinkLike`, the transformer receives **callables** instead of wire objects:

```cpp
struct CompatibleField {
    using wire_type = WireSink<1024>;
    StoredT value;
    
    bool transform_from(const auto& parseFn) {
        // parseFn is callable: bool(T& obj)
        // Attempts to parse into any type using pre-configured reader
        NewType new_val;
        if (parseFn(new_val)) {
            value = convert(new_val);
            return true;
        }
        
        // Fallback to old format
        OldType old_val;
        if (parseFn(old_val)) {
            value = convert_legacy(old_val);
            return true;
        }
        return false;
    }
    
    bool transform_to(const auto& serializeFn) const {
        // serializeFn is callable: bool(const T& obj)
        // Serializes using pre-configured writer
        NewType wire_val = convert(value);
        return serializeFn(wire_val);
    }
};
```

**How it works**:

**Parse path**:
```cpp
// 1. Reader calls capture_to_sink(WireSink)
//    - JSON: stores raw bytes
//    - YyJSON: stores node pointer (O(1))
// 2. Parser calls transform_from(parseFn)
//    - parseFn = [&](T& obj) { 
//        auto reader = Reader::from_sink(sink);
//        return ParseWithReader(obj, reader);
//      }
// 3. Transformer tries multiple wire formats
```

**Serialize path**:
```cpp
// 1. Parser calls transform_to(serializeFn)
//    - serializeFn = [&](const T& obj) {
//        auto writer = Writer::from_sink(sink);
//        auto result = SerializeWithWriter(obj, writer);
//        writer.finish();  // Populates sink
//        return result;
//      }
// 2. Transformer produces wire format
// 3. Writer calls output_from_sink(WireSink)
//    - JSON: copies raw bytes
//    - YyJSON: copies DOM node (O(n)), sink owns document via RAII
```

**Key properties**:
- Protocol-agnostic: same transformer works with JSON, YyJSON, CBOR
- O(1) for DOM: YyJSON stores `[doc*, node*]` handles
- RAII: WireSink cleanup callback manages resources (e.g., frees yyjson documents)
- Immutable: WireSink reusable, multiple `output_from_sink` calls allowed

**Use cases**:
- Schema evolution (try new format, fall back to old)
- Multi-version compatibility
- Format migrations with backward compatibility

## Full Composability

Because `wire_type` can be **any** `ParsableValue`, transformers compose naturally:

### Multi-field Transformations

```cpp
struct AddressWire {
    A<std::string, key<"street">> street;
    A<std::string, key<"city">>   city;
    A<std::string, key<"zip">>    zip;
};

struct Address {
    std::string full_address;
    
    static bool from(Address& dst, const AddressWire& wire) {
        dst.full_address = wire.street.value + ", " + 
                          wire.city.value + " " + 
                          wire.zip.value;
        return true;
    }
    // ... to() implementation ...
};

// Transform entire struct to single string
using AddressTransform = transformers::Transformed<
    Address,
    AddressWire,
    &Address::from,
    &Address::to
>;
```

### Nested Transformers

```cpp
// Date transformation
using DateStr = transformers::Transformed<Date, string, &parse_date, &format_date>;

// Range with two transformed dates
struct DateRange {
    A<DateStr, key<"start">> start;
    A<DateStr, key<"end">>   end;
};

// Transform range to period
using PeriodTransform = transformers::Transformed<
    Period,
    DateRange,
    &range_to_period,
    &period_to_range
>;
```

### Arrays of Transformed Elements

```cpp
// Each element transformed
std::vector<transformers::Transformed<UUID, string, &parse_uuid, &format_uuid>>
```

### Optional Transformations

```cpp
// Nullable transformed value
std::optional<transformers::Transformed<Money, double, &from_cents, &to_cents>>
```

### Transformers Inside Transformers

```cpp
struct InnerWire {
    A<DateStr, key<"when">> timestamp;  // Transformed date
    // ...
};

// Outer transformer's wire_type contains inner transformer!
using OuterTransform = transformers::Transformed<Outer, InnerWire, &f, &g>;
```

**Why this works**: The parser/serializer recursively calls `ParseValue` / `SerializeValue` on `wire_type`, so the type system handles all composition automatically. No special cases needed!

## Relationship with Streamers

Transformers and [streamers](./JSON_TYPE_MAPPING.md#streaming-interfaces) work beautifully together. In fact, **`ArrayReduceField` is built on top of the streaming interface**:

### ArrayReduceField Implementation

```cpp
template<class ElemWire, class StoredT, auto ReduceFn>
struct ArrayReduceField {
    // Define a ConsumingStreamerLike
    struct Streamer {
        using value_type = ElemWire;
        StoredT state{};
        
        void reset() { state = StoredT{}; }
        bool consume(const value_type& v) {
            return std::invoke(ReduceFn, state, v);
        }
        bool finalize(bool ok) { return ok; }
    };
    
    // Use streamer as wire_type - parser treats it as an array!
    using wire_type = Streamer;
    StoredT value{};
    
    bool transform_from(const wire_type& w) {
        value = w.state;
        return true;
    }
};
```

This demonstrates the **layered architecture**:

1. **Bottom layer**: `ParseTransformerLike` / `SerializeTransformerLike` concepts (baked into parser/serializer)
2. **Middle layer**: Streaming interfaces (`ConsumingStreamerLike`, `ProducingStreamerLike`)
3. **Top layer**: Convenience templates (`Transformed`, `ArrayReduceField`, `LambdaStreamer`)

### Comparison: ArrayReduceField vs LambdaStreamer

`ArrayReduceField` is conceptually similar to `LambdaStreamer` + external context:

**LambdaStreamer approach**:

```cpp
struct MyContext {
    int sum = 0;
};

auto summer = [](MyContext* ctx, int val) {
    ctx->sum += val;
    return true;
};

struct Data {
    A<LambdaStreamer<summer>, key<"values">> values;
};

// Context passed to Parse()
MyContext ctx;
Parse(data, json, &ctx);
// ctx.sum contains result
```

**ArrayReduceField approach**:

```cpp
auto summer = [](int& sum, int val) {
    sum += val;
    return true;
};

using SumField = ArrayReduceField<int, int, summer>;

struct Data {
    A<SumField, key<"values">> values;
};

// Result stored in field
Parse(data, json);
// data.values.value contains result
```

**Key differences**:
- `LambdaStreamer`: State lives in external context (useful for cross-field logic)
- `ArrayReduceField`: State lives in the field itself (useful for self-contained reduction)
- Both use the same underlying streaming mechanism
- Both are zero-overhead abstractions

## Why These Building Blocks Are Sufficient

The three templates cover all transformation patterns:

### 1. Bijective Transformations → `Transformed`
Any 1-to-1 mapping between types:
- Type conversions
- Encoding/decoding  
- Validation
- Format migrations
- Multi-field combinations

### 2. Array Reductions → `ArrayReduceField`
Any operation that processes array elements without storing the full array:
- Aggregations
- Validation-only
- Streaming statistics
- Memory-efficient processing

### 3. Map Reductions → `MapReduceField`
Any operation that processes map entries without storing the full map:
- Same as arrays, but for key-value pairs

### What About Everything Else?

**Composition handles it:**

- **Conditional logic?** Implement it in your `FromFn` / `ToFn`
- **Stateful transformations?** Store state in `StoredT`
- **Multi-field dependencies?** Use struct-level `wire_type` or external context with streamers
- **Type coercion?** That's just a `Transformed`
- **Default values?** Handle in your transformation function
- **Schema versioning?** Use `wire_type` = old schema, `StoredT` = new schema

The building blocks are **complete** - any transformation can be expressed by:
1. Choosing the right `wire_type` (might be a struct, array, primitive, etc.)
2. Writing `transform_from` / `transform_to` logic
3. Composing transformers if needed

## Summary

**Core Hooks** (in parser/serializer):
- `ParseTransformerLike` concept + specialized `ParseValue` overload
- `SerializeTransformerLike` concept + specialized `SerializeValue` overload

**Building Blocks** (in library):
- `Transformed<Stored, Wire, From, To>` - bidirectional 1-to-1
- `ArrayReduceField<Elem, Stored, Reduce>` - array → value
- `MapReduceField<Key, Val, Stored, Reduce>` - map → value *(planned)*

**Why It Works**:
- `wire_type` is fully recursive (can be any parsable/serializable type)
- Zero special cases in the core
- Full compile-time composition
- Zero runtime overhead

**Philosophy**:
- Library provides **building blocks**, not a **catalog**
- Users define transformations for their domain
- Keeps JsonFusion minimal and flexible
- Transformers compose naturally with streamers and all other features
