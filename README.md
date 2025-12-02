# JsonFusion, yet another C++ JSON library

disclaimer: under heavy development

Parse JSON directly into your structs with validation and no glue code.


#### Strongly typed, macro-free, codegen-free, with no allocations inside the library, zero-recursion on fixed-size containers, single-pass, high-performance JSON parser/serializer with declarative models and validation

Your C++ types are the schema.
JsonFusion generates a specialized parser for them at compile time.

## Motivating example

```cpp
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>

using JsonFusion::A;

struct Config {
    struct Network {
        std::string                name;
        std::string                address;

        A<int, range<8000, 9000> > port;
/*      ↑ more on this later ↑       */
    };

    Network network;

    struct Drive {
        int   id;
        float max_speed;
        bool  active;
    };
    std::vector<Drive> drives;
};

Config conf;

std::string input = R"({"drives":[{"id":0,"max_speed":4.1,"active":true}],
                    "network":{"name":"1","address":"0.0.0.0","port":8081}})";

auto parseResult = JsonFusion::Parse(conf, input);
// error checking omitted for brevity

std::string output;
JsonFusion::Serialize(conf, output);

```
*No registration macros , no JSON DOM, no inheritance*

| JSON Type | C++ Type                                              |
|-----------|-------------------------------------------------------|
| object    | `struct`,  `map<string, Any>`, *streamers*            |
| array     | `list<...>`, `vector<...>`, `array<...>`, *streamers* |
| null      | `optional<...>`, `unique_ptr<...>`                    |
| string    | `string`, `vector<char>`, `array<char, N>`, ...       |
| number    | `int`*(all kinds)*, `float`, `double`                 |
| bool      | `bool`                                                |


## Table of Contents

- [Installation](#installation)
- [Main Features](#main-features)
- [Types as Performance Hints](#types-as-performance-hints)
- [Positioning](#positioning)
  - [No Extra "Mapping" Layer](#no-extra-mapping-and-validation-handwritten-layer-with-better-performance)
  - [You Own Your Containers](#you-own-your-containers)
  - [Embedded-Friendliness](#embedded-friendliness)
  - [C Interoperability](#c-interoperability)
  - [Comparison with Glaze and reflect-cpp]()
- [Declarative Schema and Runtime Validation](#declarative-schema-and-runtime-validation)
  - [Supported Options](#supported-options-include)
- [Limitations](#limitations)
- [Benchmarks](#benchmarks)
- [Advanced Features](#advanced-features)
  - [Constexpr Parsing & Serialization](#constexpr-parsing--serialization)
  - [Streaming Producers & Consumers (Typed SAX)](#streaming-producers--consumers-typed-sax)
  - [Compile-Time Testing](#compile-time-testing)

## Installation

JsonFusion is a **header-only library**. Simply copy the include/ directory into your project’s include path (e.g. so `#include <JsonFusion/parser.hpp>` works and `pfr.hpp` is visible).


### Requirements

- **C++23** or later
- Compiler: GCC 12+, Clang 13+, MSVC 2019+
- **Boost.PFR** (bundled into `/include`, no separate installation needed)

## Main features

- **No glue code**: The main motivation behind the design is to make working with JSON similar to 
how it is usually done in Python, Java, Go, etc..
- **High performance**: ~50% faster than RapidJSON + hand-written mapping code in real-world parse-validate-populate
 workflows (see [Benchmarks](#benchmarks)). What would take hundreds and thousands  lines of 
 manual mapping/validation code collapses into a single `Parse()` call—you just define your structs 
 (which you'd need anyway) and JsonFusion handles the rest.
- The implementation conforms to the JSON standard (including arbitrary field order in objects)
- Validation of JSON shape and structure, field types compatibility and schema, all done in a single parsing pass
- No macros, no codegen, no registration – relies on PFR-driven introspection
- Works with deeply nested structs, arrays, strings, and arithmetic types out of the box
- No data-driven recursion in the parser: recursion depth is bounded by your C++ type nesting, not by JSON depth. With only
 fixed-size containers, there is no unbounded stack growth.
- **Rich error reporting**: Diagnostics with JSON path tracking (e.g., `$.statuses[3].user.name`),
input iterator position, parse error codes, and validator error codes with failed constraint details. Path tracking uses
compile-time sized storage based on schema depth analysis (zero runtime allocation overhead). Works in both runtime
and constexpr contexts. Cyclic recursive types can opt into dynamic path tracking via macro configuration. [Docs](docs/ERROR_HANDLING.md)
- **Escape hatches**: Use `json_sink<>` to capture raw JSON text when structure is unknown at compile time (plugins, pass-through, deferred parsing). JsonFusion validates JSON correctness while preserving the original fragment as a string, bridging typed and untyped worlds when needed.

## Types as Performance Hints

Your type definitions aren't just schema—they're compile-time instructions to the parser.

Assume you want to count primitives in GeoJSON data, but don't need actual values. With JsonFusion you can model coordinates pair as
```cpp
struct Pt_ {
    A<float, skip_json<>> x;
    A<float, skip_json<>> y;
};
using Point = A<Pt_, as_array>;
```
- On canada.json (2.15 MB classic benchmarking data), a hand-written RapidJSON SAX handler that counts 
features/rings/points runs in **~3.4 ms/iteration** on test machine.
- JsonFusion Streaming-based objects counting
runs in ~1.8 ms/iteration — **1.8× faster** — simply because the type system tells the parser "these values exist, 
but we don't need them." JsonFusion skips float parsing entirely while still validating the JSON structure.

RapidJSON's-like APIs have no way to express that intent without custom low-level parsing code; JsonFusion does it 
with a single annotation. The same declarative type system that eliminates boilerplate also exposes 
high-level control over low-level optimizations.

## Positioning

### No extra handwritten mapping/validation layer, often with better performance

Traditional setups use a fast JSON parser (RapidJSON, simdjson, etc.) and then a second layer of hand-written 
mapping into your C++ types. JsonFusion fuses parsing, mapping, and validation into a single pass 
— without the manual glue code.

You would have to do this work anyway; JsonFusion just helps automate it. So it doesn’t just optimize raw string
parsing; it optimizes development effort, debugging time, and lowers the maintenance burden of endless 
boilerplate that just moves bytes around.

You can think of it as a code generator that automatically produces a good parser/serializer tailored to your models.

### You own your containers

JsonFusion never dictates what containers or strings you must use. It works with any type that behaves like a 
standard C++ container:

- strings: `std::string`, `std::array<char, N>`, your own fixed buffers…
- arrays/lists: `std::vector<T>`, `std::list<T>`, `std::array<T, N>`, etc.

JsonFusion does **not** install custom allocators, memory pools, or arenas. It simply reads JSON and writes 
into the storage you provide. 

This has a few consequences:

- You keep full control over memory behavior (heap vs stack, fixed-size vs dynamic).
- Integration with existing C++ and even C structs is straightforward.
- There are no hidden allocation tricks inside the library.
- There are very few global options/knobs-not needed.

It also explains part of the performance story: libraries like RapidJSON use highly tuned internal DOM structures and custom allocators, which can squeeze out more speed for dynamic, DOM-heavy workloads. JsonFusion chooses instead to be a thin, strongly typed layer over *your* containers, while still staying competitive in raw parsing speed.

### Embedded-friendliness

No data-driven recursion in the parser core — depth is bounded by your user-defined types and is explicit.

No dynamic allocations inside the library. You choose your storage: `std::array`, fixed-size char buffers, or dynamic containers (`std`-compatible).

Works with forward-only input/output iterators: you can parse from a stream or byte-by-byte without building a big buffer first, and serialize byte-by-byte with full control and no intermediate buffers.

Plays well with -fno-exceptions / -fno-rtti style builds.

**Configurable dependencies**: By default uses external fast float parsers/serializers. Define `JSONFUSION_USE_FAST_FLOAT=0` to depend only on PFR + C stdlib for floats handling, reducing binary size for embedded builds.

**Your structs define everything**: On tiny microcontrollers without floating-point support, just don’t use float/double in your struct definitions. In that case the float parsing code is never instantiated or compiled into the binary; there is no special global configuration needed. The same applies to dynamic lists/strings.

### C Interoperability

Add JSON parsing to legacy C codebases **with very small or without modifying your C headers**. JsonFusion parsing can live in a separate C++ translation unit and will work with plain C structs just fine:

```c
// structures.h - Pure C header, unchanged
typedef struct {
    int device_id;
    float temperature;
    SensorConfig sensor; // SensorConfig is another C struct
} DeviceConfig;
```

```cpp
// parser.cpp - C++ wrapper with C API
extern "C" int ParseDeviceConfig(DeviceConfig* config, 
                                 const char* json_data, 
                                 size_t json_size) {
    auto result = JsonFusion::Parse(*config, json_data, json_data + json_size);
    return result ? 0 : static_cast<int>(result.error());
}
```

```c
// main.c - Pure C usage
DeviceConfig config;
int error = ParseDeviceConfig(&config, json, strlen(json));
```
If you define custom input/output iterators, you can integrate JSON parsing into virtually any C code base.
A good fit for:
- **Legacy firmware** - Add JSON without touching existing C code too much
- **Embedded C projects** - Modern JSON parsing with zero C code changes

**String handling**: When parsing into fixed-size char arrays (common in C structs), JsonFusion automatically null-terminates strings, making them immediately usable with standard C string functions (`strlen`, `strcmp`, `printf`, etc.).

**Note:** C arrays (`int arr[10]`) aren't supported due to PFR limitations, but primitives and nested structs work 
perfectly. See `examples/c_interop/` for a complete working example.

All PFR usage is isolated in [`struct_introspection.hpp`](include/JsonFusion/struct_introspection.hpp), which defines 
"how to iterate struct fields" as an abstraction layer. The rest of JsonFusion only talks to that abstraction. When 
C++26 reflection becomes mainstream, this layer can be replaced with a standards-based implementation without changing 
JsonFusion's public API or core design. Better reflection support will only improve capabilities (including more 
seamless C interop) without forcing users to rewrite code. See the [philosophy](docs/WHY_REFLECTION.md) for details.


## Declarative Schema and Runtime Validation

Declarative compile-time schema and options, runtime validation in the same parsing single pass.
Turn your C++ structs into a static schema, decouple variable names from JSON keys, and add parsing rules by annotating fields.

**Full reference:** [Annotations Reference](docs/ANNOTATIONS_REFERENCE.md)

```cpp
using JsonFusion::A;
using namespace JsonFusion::options;
using std::string; using std::vector, std::optional;
struct GPSTrack {
    A<int,
        key<"id">,
        range<1, 8>>             more_convenient_descriptive_id;

    A<string,
        min_length<1>,
        max_length<32>>          name;

    A<bool, not_json>    m_is_handled;

    struct Point {
        float x;
        float y;
        float z;
    };
    A<std::vector<
            A<Point, as_array>>,
        max_items<4096>>         points;
};

list<optional<GPSTrack>> tracks;
JsonFusion::Parse(tracks, string_view(R"JSON(
    [
        null,
        {"name": "track1", "id": 1, "points": [[1,2,3], [4,5,6], [7,8,9]]},
        null
    ]
)JSON"));
```

### Behaviour of Annotated<> (has A<> alias) class
Tiny wrapper around your types, without inheritance, but with some glue to make it work as "natural" as possible.

```cpp
auto handlePoint = [](const GPSTrack::Point & p){};
for (const auto & track: tracks) {
    if(!track) continue;
    for (const auto & point: track->points) { // implicit conversions & operator forwarding
        handlePoint(point);
    }
}
```

### Supported Options Include

JsonFusion provides validators (runtime constraints) and options (metadata/behavior control):

- **Validators**: `range<>`, `min_length<>`, `max_length<>`, `enum_values<>`, `min_items<>`, `max_items<>`, `constant<>`, and more
- **Options**: `key<>`, `not_json`, `skip_json<>`, `json_sink<>`, `as_array`, `allow_excess_fields<>`, and more

See the complete reference: [Annotations Reference](docs/ANNOTATIONS_REFERENCE.md)

## Limitations

- Designed for C++23 aggregates (POD-like structs). Classes with custom constructors, virtual methods, etc. are not automatically reflectable
- Relies on PFR; a few exotic compilers/ABIs may not be supported.
- **`THIS IS NOT A JSON DOM LIBRARY.`** It shines when you have a known schema and want to map JSON directly from/into C++ types; if you need a generic JSON tree and ad-hoc editing, JsonFusion is not the right tool; consider using it alongside a DOM library.

## Benchmarks

JsonFusion targets two distinct scenarios with different priorities:

- **Embedded systems** – where binary size is critical (measured in KB)
- **High-performance systems** – where throughput is critical (measured in µs)

**Performance philosophy**: JsonFusion tries to achieve both speed and compactness by 
*eliminating unnecessary work*, rather than through manual micro-optimizations. 
The core is platform/CPU agnostic (no SIMD, no hand-tuned assembly). 
With `JSONFUSION_USE_FAST_FLOAT=0`, the entire stack external runtime dependency is portable C stdlib strod/snprintf. 

Less work means both faster execution *and* smaller binaries.
It is all about avoiding doing the same work multiple times.

JsonFusion leverages **compile-time reflection** through Boost.PFR, enabling the compiler to know everything about your types before runtime.  This isn't a hack – C++26 is expected to standardize native reflection, making this approach future-proof.

### Binary Size (Embedded Focus)

*Benchmarks coming soon* – measuring stripped binary size on ARM Cortex-M platforms with minimal configs.



### Parsing Speed (High-Performance Focus)

**What we're measuring:** The complete real-world workflow of parsing JSON and populating C++ structs with validation. This is what you actually do in production code—not abstract JSON DOM manipulation or raw string parsing in isolation.

Both libraries perform the same work:
1. Parse JSON from string
2. Validate all constraints (type compatibility, ranges, array sizes, string lengths, enum values)
3. Populate C++ structures with the data

For RapidJSON, this requires hand-written mapping code. For JsonFusion, it's automatic via reflection.

📁 **Test models**: [`benchmarks/main.cpp`](benchmarks/main.cpp) – 6 realistic scenarios with nested structs, arrays, maps, optionals, validation constraints  

#### Results

| Benchmark Scenario | JsonFusion | RapidJSON | Speedup |
|-------------------|------------|-----------|---------|
| **Embedded Config (Static)** | 1 µs | 1.3 µs | **~30% faster** |
| **Embedded Config (Dynamic)** | 1.2 µs | 2 µs | **~70% faster** |
| **Telemetry Samples** | 4.9 µs | 7.8 µs | **~60% faster** |
| **RPC Commands** | 2.3 µs | 3.9 µs | **~60% faster** |
| **Log Events** | 3.5 µs | 5 µs | **~50% faster** |
| **Bus Events / Message Payloads** | 4.9 µs | 7.7 µs | **~60% faster** |
| **Metrics / Time-Series** | 4.3 µs | 6 µs | **~40% faster** |

*Tested on Apple M1 Max, macOS 26.1, GCC 15, 1M iterations per scenario*

#### Key Takeaways

**1. Generic template metaprogramming beats hand-written low-level code**

The **Embedded Config (Static)** benchmark is particularly interesting: it uses RapidJSON's SAX parser (the lowest-level, most performance-oriented API) with hand-written state machine and direct struct population—zero DOM allocations, maximum control. Yet JsonFusion's generic reflection-based approach is **~30% faster** while requiring zero mapping code.

This demonstrates that compile-time reflection isn't just convenient—it enables optimizations that are difficult to achieve with manual code.

**2. Consistent performance advantage across all scenarios**

JsonFusion wins every benchmark by 30-70%, with larger gains on:
- Complex nested structures (70% on dynamic embedded config)
- Maps and validation-heavy workloads (60% on telemetry, RPC, bus events)
- Smaller but still significant gains on simpler structures (40% on metrics)

**3. Static vs dynamic containers**

Both libraries show performance differences between static (`std::array`) and dynamic (`std::vector`, `std::string`) containers, but JsonFusion maintains its lead in both cases:
- Static: 1 µs vs 1.4 µs (30% faster)
- Dynamic: 1.2 µs vs 2 µs (70% faster)

The larger dynamic advantage suggests JsonFusion's single-pass design particularly benefits scenarios with memory allocation.

**4. Fair comparison**

The RapidJSON benchmark represents typical usage: parse JSON into DOM, then walk the tree to populate and validate the target C++ structures. For the static embedded case, we use SAX-style parsing with a hand-written state machine to eliminate DOM overhead entirely. Both approaches do the same work as JsonFusion—this is an apples-to-apples comparison of complete parse-validate-populate workflows, not raw parsing speed.

**5. Large file performance: canada.json (2.2 MB, 117K+ coordinates)**

The canada.json benchmark tests two distinct workflows on a numeric-heavy GeoJSON file—a pure array/number stress test rather than typical structured configs.

**Classic parse-validate-populate** (production use case):
- **JsonFusion Parse + Populate**: 5350 µs  
- **RapidJSON DOM Parse + Populate**: 5650 µs (~5% slower)

**JsonFusion wins** where it matters—getting validated C++ structs ready to use.

**Streaming API for counting objects** :
- **RapidJSON SAX** (hand-written state machine): 3400 µs  
- **JsonFusion Streaming** : 5150 µs (~50% slower)

Here RapidJSON SAX wins—it's essentially the fastest possible approach for this simple, regular data. But consider: JsonFusion's typed streaming handles canada.json's trivial structure **the exact same way** it would handle complex schemas like twitter.json—where writing equivalent SAX code becomes prohibitively complex. JsonFusion trades ~2ms on this worst-case scenario for a **fully generic, type-safe, composable API** with zero manual state machines. No CPU hacks, just portable forward-only iterator code.
But next
- **JsonFusion Streaming without materializing floats** : 3400 µs (same speed as handwritten SAX RapidJSON) Tokenize and validate JSON correctness, but don't parse. Not needed for counting objects, still basic correctness is validated
- **JsonFusion Streaming with skipping each of 2 float numbers inside points** : 1800 µs (**190% faster than handwritten SAX RapidJSON**) Just don't parse, if don't need the value.



📁 **Canada.json benchmark**: [`benchmarks/canada_json_parsing.cpp`](benchmarks/canada_json_parsing.cpp)

**6. Structured data performance: twitter.json (0.6 MB, 100 status objects)**

The twitter.json benchmark tests realistic structured data with deeply nested objects, optional fields, and mixed types—representative of real-world REST API responses and configurations.

**Results (same hardware):**
- **RapidJSON DOM Parse only**: 780 µs (baseline, but unusable—no typed data)
- **JsonFusion Parse + Populate**: 1060 µs (**35% overhead over DOM-only**, fully typed)
- **RapidJSON Parse + Manual Populate**: 1470 µs (40% slower than JsonFusion)

**Key insight**: JsonFusion is **~38% faster** than the "traditional" approach (RapidJSON DOM + manual mapping), while requiring **zero boilerplate**.

The manual RapidJSON populate code ([`rapidjson_populate.hpp`](benchmarks/rapidjson_populate.hpp)) is **450+ lines** of hand-written DOM traversal and type mapping—exactly the kind of tedious, error-prone code JsonFusion eliminates. 

📁 **Twitter.json benchmark**: [`benchmarks/twitter_json_parsing.cpp`](benchmarks/twitter_json_parsing.cpp)  
📁 **Data model**: [`benchmarks/twitter_model.hpp`](benchmarks/twitter_model.hpp)
📁 **Manual populate code**: [`benchmarks/rapidjson_populate.hpp`](benchmarks/rapidjson_populate.hpp) 

## Advanced Features

### Constexpr Parsing & Serialization

For models using compatible containers (`std::string`, `std::vector` are compatible) and  types (everything except FP numbers), both
`Parse` and `Serialize` are fully `constexpr`-compatible. This enables compile-time JSON 
validation, zero-cost embedded configs, and proves no hidden allocations or runtime 
dependencies. See the compile-time test suite [`tests/constexpr/*`](tests/constexpr) for 
examples with nested structs, arrays, and optionals.

### Streaming Producers & Consumers (Typed SAX)

Any JSON array field can be bound to either a **container** (`std::vector<T>`, `std::array<T,N>`) or a **streaming producer/consumer**.

#### Streaming Producers (for Serialization)

Instead of materializing all elements in memory, fill them on demand:

```cpp
struct Streamer {
    struct Vector { float x, y, z; };
    using value_type = A<Vector, as_array>;

    mutable int counter = 0;
    int count;

    void reset() const { counter = 0; }
    
    stream_read_result read(Vector & v) const {
        if (counter >= count) return stream_read_result::end;
        counter++;
        v = {42.0f + counter, 43.0f + counter, 44.0f + counter};
        return stream_read_result::value;
    }
};
static_assert(ProducingStreamerLike<Streamer>);

struct TopLevel {
    Streamer points_xyz;  // Will serialize as JSON array
};

TopLevel a;
a.points_xyz.count = 3;
std::string out;
Serialize(a, out);
// {"points_xyz":[[43,44,45],[44,45,46],[45,46,47]]}
```

JsonFusion holds a **single `value_type` buffer on the stack** and repeatedly calls `read()`. 
No intermediate containers, no hidden allocations.

**Composability**: Since `value_type` can be any JsonFusion-compatible value (primitives, structs, 

`Annotated<>`, even other streamers), you can nest producers arbitrarily:

```cpp
struct StreamerOuter {
    struct StreamerInner {
        using value_type = double;
        // ... fills doubles on demand
    };
    using value_type = StreamerInner;  // Stream of streamers!
    // ... fills  StreamerInner instances state on demand
};

Serialize(StreamerOuter{}, out);
// [[1],[1,2],[1,2,3],[1,2,3,4],...]  – 2D jagged array, zero materialized containers
```

#### Streaming Consumers (for Parsing)

Consume elements as they arrive, without storing the whole array:

```cpp
struct PointsConsumer {
    using value_type = Vector;

    void reset() { /* called at array start */ }
    
    bool consume(const Vector & point) {
        processPoint(point);  // handle element immediately
        return true;
    }
    
    bool finalize(bool success) { 
        return success;  // called after last element
    }
};
static_assert(ConsumingStreamerLike<PointsConsumer>);

struct TopLevel {
    A<PointsConsumer, key<"points">> consumer;
};

Parse(toplevel, R"({"points": [[1,2,3], [4,5,6], ...]})");
// consume() called for each element, no std::vector allocated
```

JsonFusion parses each element directly into a **stack-allocated `value_type` buffer**, calls `consume()`, then reuses the buffer for the next element.

#### Why This Is Different from Classic SAX

Classic SAX APIs (RapidJSON, simdjson) drive you with low-level events:
```
StartArray, Key("foo"), String("bar"), Int(42), EndObject, …
```
You manually maintain state and assemble typed objects yourself.

**JsonFusion's approach:**
- You declare `value_type` (using the same `Annotated<>` system, just as for normal fields)
- JsonFusion parses each element into a **fully-typed C++ object**
- Your callbacks receive complete, validated structures—not raw tokens
- The abstraction **composes naturally**—streamers can contain structs, which contain other streamers
- API to pass local (both at types and runtime level) typed context pointer via ParseWithContext/SerializeWithContext
No need to use any global variables/singletons

**Unified mechanism:**
- **Producers**: `read()` fills elements on demand (serialization)
- **Consumers**: `consume()` receives fully-parsed elements (parsing) at the moment last input char of primitive parsed
- Library uses only a single buffer object of corresponding type; all dynamic memory is under your control

📁 **Complete examples**: [`tests/sax_demo.cpp`](tests/sax_demo.cpp) including nested streamer composition, 
[`benchmarks/canada_json_parsing.cpp`](benchmarks/canada_json_parsing.cpp) for Context Propagation.
### Compile-Time Testing

JsonFusion's core is tested primarily through `constexpr` tests—JSON parsing and serialization executed entirely at **compile time** using `static_assert`. No test framework, no runtime: the compiler is the test runner.

**Why constexpr tests:**
- **Proves zero hidden allocations**: If it compiles in `constexpr`, no dynamic allocation happened
- **Validates correctness before any runtime**: Catches bugs at compile time
- **Tests the actual user-facing API**: Same `Parse`/`Serialize` calls users make


**Example** (this runs at compile time):
```cpp
struct Config { 
    int value;
    std::string dynamic_string;
    std::vector<std::vector<int>> nested_vecs;
    std::vector<std::optional<std::vector<std::string>>> complex_nested;
};

static_assert([]() constexpr {
    Config c{};
    auto result = JsonFusion::Parse(c, std::string(R"(
        {
            "value": 42,
            "dynamic_string": "allocated at compile time!",
            "nested_vecs": [[1,2], [3,4,5]],
            "complex_nested": [null, ["a","b"], null]
        })"));
    return result 
        && c.value == 42 
        && c.nested_vecs[1][2] == 5
        && c.complex_nested[1]->at(0) == "a";
}()); 
```

This approach aligns with JsonFusion's philosophy: leverage compile-time introspection to prove correctness before the program even runs.

📁 **Test suite details**: [`tests/constexpr/README.md`](tests/constexpr/README.md)
