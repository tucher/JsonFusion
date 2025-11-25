# JsonFusion, yet another C++ JSON library

Parse JSON directly into your structs with validation and no glue code.


#### Strongly typed, macro-free, codegen-free, with no allocations inside the library, zero-recursion on fixed-size containers, single-pass, high-performance JSON parser/serializer with declarative models and validation

## Motivating example

```cpp
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>

struct Config {
    struct Network {
        std::string name;
        std::string address;
        int port = 8080;
    };

    Network network;

    struct Drive {
        int   id;
        float max_speed = 0;
        bool active = false;
    };
    std::vector<Drive> drives;
};

Config conf;

std::string input = R"({"drives":[{"id":0,"max_speed":4.1,"active":true}],"network":{"name":"1","address":"0.0.0.0","port":8081}})";
auto parseResult = JsonFusion::Parse(conf, input);
// error checking omitted for brevity

std::string output;
JsonFusion::Serialize(conf, output);

```
*No macros, no registration, no JSON DOM, no inheritance or wrapping*

| JSON Type | C++ Type                                              |
|-----------|-------------------------------------------------------|
| object    | `struct`  *is_aggregate_v, all kinds*                 |
| array     | `list<...>`, `vector<...>`, `array<...>`, *streamers* |
| null      | `optional<...>`                                       |
| string    | `string`, `vector<char>`, `array<char, N>`, ...       |
| number    | `int`*(all kinds)*, `float`, `double`                 |
| bool      | `bool`                                                |

**Note on strings**: When parsing into fixed-size char arrays (`std::array<char, N>`), JsonFusion **always null-terminates** the string (if space permits), making them directly usable with C string functions like `strlen()`, `printf()`, etc. This is verified in compile-time tests.


## Table of Contents

- [Installation](#installation)
- [Main Features](#main-features)
- [Performance](#performance)
- [Positioning](#positioning)
  - [No Extra "Mapping" Layer](#no-extra-mapping-and-validation-handwritten-layer-with-same-performance)
  - [You Own Your Containers](#you-own-your-containers)
  - [Embedded-Friendliness](#embedded-friendliness)
  - [C Interoperability](#c-interoperability)
- [Declarative Schema and Runtime Validation](#declarative-schema-and-runtime-validation)
  - [Supported Options](#supported-options-include)
- [Limitations](#limitations)
- [Benchmarks](#benchmarks)
- [Advanced Features](#advanced-features)
  - [Constexpr Parsing & Serialization](#constexpr-parsing--serialization)
  - [Streaming Producers & Consumers (Typed SAX)](#streaming-producers--consumers-typed-sax)

## Installation

JsonFusion is a **header-only library**. Simply copy the include/ directory into your project’s include path (e.g. so `#include <JsonFusion/parser.hpp>` works and `pfr.hpp` is visible).


### Requirements

- **C++23** or later
- Compiler: GCC 12+, Clang 13+, MSVC 2019+
- **Boost.PFR** (bundled into `/include`, no separate installation needed)

## Main features

- High performance, comparable to RapidJSON + manual mapping
- The implementation conforms to the JSON standard (including arbitrary field order in objects)
- Validation of JSON shape and structure, field types compatibility and schema, all done in s single parsing pass
- No macros, no codegen, no registration – relies on PFR and aggregates
- Works with deeply nested structs, arrays, strings, and arithmetic types out of the box
- No data-driven recursion in the parser: recursion depth is bounded by your C++ type nesting, not by JSON depth. With only fixed-size containers, there is no unbounded stack growth.
- Error handling via a result object convertible to bool, with access to an error code and offset. C++ exceptions are not used.

## Performance

### Competitive with hand-written RapidJSON code
Benchmarks on realistic configs show JsonFusion within the same range as RapidJSON + manual mapping, and sometimes faster, while avoiding the DOM and hand-written conversion code.

## Positioning

### No extra handwritten mapping/validation layer, with the same performance

Traditional setups use a fast JSON parser (RapidJSON, simdjson, etc.) and then a second layer of hand-written mapping into your C++ types. JsonFusion fuses parsing, mapping, and validation into a single pass, so it behaves like a thin, typed layer on top of a fast parser — without the manual glue code.

You would have to do this work anyway; JsonFusion just helps automate it. So it doesn’t just optimize raw string parsing; it optimizes development effort, debugging time, and lowers the maintenance burden of endless boilerplate that just moves bytes around.

You can think of it as a code generator that automatically produces a good parser/serializer tailored to your models.

### You own your containers

JsonFusion never dictates what containers or strings you must use. It works with any type that behaves like a standard C++ container:

- strings: `std::string`, `std::array<char, N>`, your own fixed buffers…
- arrays/lists: `std::vector<T>`, `std::list<T>`, `std::array<T, N>`, etc.

JsonFusion does **not** install custom allocators, memory pools, or arenas. It simply reads JSON and writes into the storage you provide.

This has a few consequences:

- You keep full control over memory behavior (heap vs stack, fixed-size vs dynamic).
- Integration with existing C++ and even C structs is straightforward.
- There are no hidden allocation tricks inside the library.

It also explains part of the performance story: libraries like RapidJSON use highly tuned internal DOM structures and custom allocators, which can squeeze out more speed for dynamic, DOM-heavy workloads. JsonFusion chooses instead to be a thin, strongly typed layer over *your* containers, while still staying competitive in raw parsing speed.

### Embedded-friendliness

No data-driven recursion in the parser core — depth is bounded by your user-defined types and is explicit.

No dynamic allocations inside the library. You choose your storage: `std::array`, fixed-size char buffers, or dynamic containers (`std`-compatible).

Works with forward-only input/output iterators: you can parse from a stream or byte-by-byte without building a big buffer first, and serialize byte-by-byte with full control and no intermediate buffers.

Plays well with -fno-exceptions / -fno-rtti style builds.

**Configurable dependencies**: By default uses state-of-the-art float parsers/serializers. Define `JSONFUSION_USE_FAST_FLOAT=0` to depend only on PFR + C stdlib for floats handling, reducing binary size for embedded builds.

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

**Note:** C arrays (`int arr[10]`) aren't supported due to PFR limitations, but primitives and nested structs work perfectly. See `examples/c_interop/` for a complete working example. See the [philosophy](docs/WHY_REFLECTION.md) - standard C++ reflection will likely change this story in the near future.


## Declarative Schema and Runtime Validation

Declarative compile-time schema and options, runtime validation in the same parsing single pass.
Turn your C++ structs into a static schema, decouple variable names from JSON keys, and add parsing rules by annotating fields:

```cpp
using JsonFusion::Annotated;
using namespace JsonFusion::options;
using std::string; using std::vector, std::optional;
struct GPSTrack {
    Annotated<int,
        key<"id">,
        range<1, 8>>             more_convenient_descriptive_id;

    Annotated<string,
        min_length<1>,
        max_length<32>>          name;

    Annotated<bool, not_json>    m_is_handled;

    struct Point {
        float x;
        float y;
        float z;
    };
    Annotated<std::vector<
            Annotated<Point, as_array>>,
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

### Supported options include:

- `key<"...">` – JSON field name
- `range<min,max>` – numeric range checks
- `min_length<N>` / `max_length<N>` – string length
- `min_items<N>` / `max_items<N>` – array size
- `not_required` – optional field without having to use std::optional<T>
- `not_json` – internal/derived fields that are invisible to JSON
- `allow_excess_fields` – per-object policy for skipping unknown fields 
- `as_array` – array destructuring for objects (treat struct as JSON array)

### Behaviour of Annotated<> class
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
With `JSONFUSION_USE_FAST_FLOAT=0`, the entire stack is fully portable C stdlib. 
Less work means both faster execution *and* smaller binaries.

It is all about avoiding doing the same work multiple times.

JsonFusion leverages **compile-time reflection** through Boost.PFR, enabling the compiler to know everything about your types before runtime.  This isn’t a hack – C++26 is expected to standardize native reflection, making this approach future-proof.

### Binary Size (Embedded Focus)

*Benchmarks coming soon* – measuring stripped binary size on ARM Cortex-M platforms with minimal configs.

**What matters here**: Total code size with JsonFusion vs alternatives, impact of `JSONFUSION_USE_FAST_FLOAT` flag, comparison with hand-written parsing.

---

### Parsing Speed (High-Performance Focus)

Benchmarked on realistic hardware config: nested structs, arrays of floats/ints, optionals, validation constraints. Two variants tested: static containers (`std::array`) vs dynamic containers (`std::vector`, `std::string`, `std::list`).

📁 **Test model**: [`tests/test_model.hpp`](tests/test_model.hpp)  
📁 **Benchmark code**: [`benchmarks/comparison_parsing_benchmark.cpp`](benchmarks/comparison_parsing_benchmark.cpp)

**What matters here**: Time to parse JSON into fully-populated, validated C++ structs.

**Important:** RapidJSON below is only parsing into a Document DOM, while JsonFusion parses directly into the final struct with validation. JsonFusion shows about the same speed as RapidJSON for static containers (sometimes slightly faster) and 25-30% slower for **`std`** dynamic containers — **but with full mapping and validation in both cases**.
(As always, benchmarks depend on the model and compiler; see the benchmark sources for details.)

#### RapidJSON DOM parsing

```
rj::Document doc;
doc.Parse(kJsonStatic.data(), kJsonStatic.size());
```
**2.42 - 2.49 µs**

#### JsonFusion with static containers + validation

```
StaticComplexConfig cfg;
Parse(cfg, kJsonStatic.data(), kJsonStatic.size());
```
**2.40 - 2.44 µs** ✨ (faster than RapidJSON DOM, with validation!)

#### JsonFusion with dynamic containers + validation

```
DynamicComplexConfig cfg;
Parse(cfg, kJsonStatic.data(), kJsonStatic.size());
```
**2.97 - 3.07 µs** (still competitive!)

*GCC 13.2, arm64 Apple M1 Max, Ubuntu Linux on Parallels VM*

## Advanced Features

### Constexpr Parsing & Serialization

For models using fixed-size containers (`std::array`, char buffers) and  types, both
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
    using value_type = Annotated<Vector, as_array>;

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
    Annotated<PointsConsumer, key<"points">> consumer;
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
- You declare `value_type` (using the same `Annotated<>` system as normal fields)
- JsonFusion parses each element into a **fully-typed C++ object**
- Your callbacks receive complete, validated structures—not raw tokens
- The abstraction **composes naturally**—streamers can contain structs, which contain other streamers

**Unified mechanism:**
- **Producers**: `read()` fills elements on demand (serialization)
- **Consumers**: `consume()` receives fully-parsed elements (parsing) at the moment last input char of primitive parsed
- Library uses only a small per-field stack buffer; all dynamic memory is under your control

📁 **Complete examples**: [`tests/sax_demo.cpp`](tests/sax_demo.cpp) including nested streamer composition
