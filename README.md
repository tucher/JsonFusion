# JsonFusion, yet another JSON library

### Strongly typed, macro-free, codegen-free, with no allocations inside, zero-recursion on fixed-sized containers, single-pass high-performance JSON parser/serializer with declarative models and validation

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

    struct Drives {
        int   id;
        float max_speed = 0;
        bool active = false;
    };
    std::vector<Drives> drives;
};

Config conf;

std::string input = R"({"drives":[{"id":0,"max_speed":4.1,"active":true}],"network":{"name":"1","address":"0.0.0.0","port":8081}})";
JsonFusion::Parse(conf, input);

std::string output;
JsonFusion::Serialize(conf, output);

```
*No macros, no registration, no JSON DOM, no inheritance or wrapping*

| JSON Type | C++ Type                                        |
|-----------|-------------------------------------------------|
| object    | `struct`  *is_aggregate_v*                      |
| array     | `list<...>`, `vector<...>`, `array<...>`, ...   |
| null      | `optional<...>`                                 |
| string    | `string`, `vector<char>`, `array<char, N>`, ... |
| number    | `int`, `float`, `double`                        |
| bool      | `bool`                                          |



## Table of Contents

- [Installation](#installation)
- [Main Features](#main-features)
- [Why Compile-Time Reflection?](#why-compile-time-reflection) 📖 [Full Article](docs/WHY_REFLECTION.md)
- [Performance](#performance)
- [Positioning](#positioning)
  - [No Extra "Mapping" Layer](#no-extra-mapping-and-validation-handwritten-layer-with-same-performance)
  - [Embedded-Friendliness](#embedded-friendliness)
  - [C Interoperability](#c-interoperability)
- [Declarative Schema and Runtime Validation](#declarative-schema-and-runtime-validation)
  - [Supported Options](#supported-options-include)
- [Limitations](#limitations)
- [Benchmarks](#benchmarks)

## Installation

JsonFusion is a **header-only library**. Simply copy the /include to your project's include directory.


### Requirements

- **C++20** or later
- Compiler: GCC 12+, Clang 13+, MSVC 2019+
- **Boost.PFR** (bundled, no separate installation needed)

## Main features

- Good performance
- The implementation conforms to the standard (specifically, arbitrary order of fields inside objects is supported)
- Validation of JSON shape and structure, field types compatibility and schema, all done in the same single parsing pass
- No macros, no codegen, no registration – relies on PFR and aggregates
- Works with deeply nested structs, arrays, strings, and arithmetic types out of the box
- No data-driven recursion in the parser: recursion depth is bounded by your C++ type nesting, not by JSON depth. With only fixed-size containers, there is no unbounded stack growth.
- Error handling via convertible to bool result object with access to error code and offset. C++ exceptions are not used.

## Why Compile-Time Reflection?

JsonFusion leverages **compile-time reflection** through Boost.PFR, enabling the compiler to know everything about your types before runtime.  This isn't a hack – **C++26 (late 2025) is standardizing native reflection**, making this approach the future of C++.

📖 **[Read the philosophy →](docs/WHY_REFLECTION.md)** – includes C++26 context,  technical benefits, and embedded systems discussion.

It is all about avoiding doing the same work multiple times.

## Performance

### Competitive with hand-written RapidJSON code
Benchmarks on realistic configs show JsonFusion within the same range as RapidJSON + manual mapping, and sometimes faster, while avoiding the DOM and hand-written conversion code.

## Positioning

### No extra “mapping” and validation handwritten layer, with same performance

Traditional setups use a fast JSON parser (RapidJSON, simdjson, etc.) and then a second layer of hand-written mapping into your C++ types. JsonFusion fuses parsing, mapping and validation into a single pass, so it behaves like a thin, typed layer on top of a fast parser — without the manual glue code.

You would have to do this work anyway, JsonFusion just help to automate. So it optimizes not the raw strings parsing, it optimizes development effort, debugging time and lowers maintainance burden of endless boilerplate lines of code that just move some bytes around.

You may see it as codegenerator, which automatically produce good parser/serializer just for your models.

### Embedded-friendliness

No data-driven recursion in the parser core — depth is bounded by user-defined types and explicit.

No dynamic allocations inside the library. You choose your storage: `std::array`, fixed-size char buffers, or dynamic containers.

Works with forward-only input/output iterators; you can parse from a stream or byte-by-byte without building a big buffer first and serialize the result byte-by-byte with full control, without intermediate buffers anywhere.

Plays well with -fno-exceptions / -fno-rtti style builds.

**Configurable dependencies**: By default uses state-of-the-art float parsers/serializers. Define `JSONFUSION_USE_FAST_FLOAT=0` to depend only on PFR + C stdlib for floats handling, reducing binary size for embedded builds.

**Your struct defines everything**: On tiny uC without floating point support just don't use the unavailable `float`/`double` in your structures definitions. Float parsing code is never instantiated or compiled into the binary in this case, there is no special global configuration. Same story for dynamic lists/strings.

### C Interoperability

Add JSON parsing to legacy C codebases **with very small or without modifying your C headers**. JsonFusion parsing can be placed into separate unit and will work with plain C structs just fine:

```c
// structures.h - Pure C header, unchanged
typedef struct {
    int device_id;
    float temperature;
    SensorConfig sensor;  // Nested structs work!
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
If you define custom input/output iterators, you will be able to plug in the JSON into any C code base.
Fits for:
- **Legacy firmware** - Add JSON without touching existing C code too much
- **Embedded C projects** - Modern JSON parsing with zero C code changes

**Note:** C arrays (`int arr[10]`) aren't supported due to PFR limitations, but primitives and nested structs work perfectly. See `examples/c_interop/` for a complete working example. But see the [philosophy](docs/WHY_REFLECTION.md), that will change very soon most likely.


## Declarative Schema and Runtime Validation

Declarative compile-time schema and options, runtime validation in the same parsing single pass.
Turn your C++ structs into a static schema, decouple variable's names from JSON keys, add parsing rules by annotating fields:

```cpp
using JsonFusion::Annotated;

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
    for (const auto & point: track->points) { // automatic implicit casts
        handlePoint(point);                   // and operators forwarding inside
    }
}
```


## Limitations

- Designed for C++20 aggregates (POD-like structs). Classes with custom constructors, virtual methods, etc. are not automatically reflectable
- Relies on PFR; a few exotic compilers/ABIs may not be supported.
- **THIS IS NOT A JSON DOM LIBRARY.** It shines when you have a known schema and want to map JSON directly from/into C++ types; if you need a generic tree & ad-hoc editing, it is not what you need.

## Benchmarks

JsonFusion targets two distinct scenarios with different priorities:

- **Embedded systems** – where binary size is critical (measured in KB)
- **High-performance systems** – where throughput is critical (measured in µs)

### Binary Size (Embedded Focus)

*Benchmarks coming soon* – measuring stripped binary size on ARM Cortex-M platforms with minimal configs.

**What matters here**: Total code size with JsonFusion vs alternatives, impact of `JSONFUSION_USE_FAST_FLOAT` flag, comparison with hand-written parsing.

---

### Parsing Speed (High-Performance Focus)

Benchmarked on realistic hardware config: nested structs, arrays of floats/ints, optionals, validation constraints. Two variants tested: static containers (`std::array`) vs dynamic containers (`std::vector`, `std::string`, `std::list`).

📁 **Test model**: [`tests/test_model.hpp`](tests/test_model.hpp)  
📁 **Benchmark code**: [`benchmarks/comparison_parsing_benchmark.cpp`](benchmarks/comparison_parsing_benchmark.cpp)

**What matters here**: Time to parse JSON into fully-populated, validated C++ structs.

**Important:** RapidJSON below is only parsing into a Document DOM, while JsonFusion parses directly into the final struct with validation. JsonFusion shows about the same speed as RapidJSON for static containers (sometimes slightly faster) and 25-30% slower for dynamic containers — **but with full mapping and validation in both cases**.

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
