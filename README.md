# JsonFusion, yet another JSON library

### Strongly typed, macro-free, codegen-free, with no allocations inside, zero-recursion on fixed-sized containers, single-pass high-performance JSON parser/serializer with declarative models and validation

## Motivating example

```cpp

struct Config {
    struct Network {
        std::string name;
        std::string address;
        int port = 8080;
    };

    Network network;

    struct Motor {
        int   id;
        float max_speed = 0;
        bool active = false;
    };
    std::array<Motor, 4> motors;
};

Config config;
if(auto result = Parse(config, input); !result) {
    std::cout << std::format("Error {} at pos {}", result.error(), result.pos()) << std::endl;
}
```
*No macros, no registration, no JSON DOM, no inheritance or wrapping*

## Table of Contents

- [Installation](#installation)
- [Main Features](#main-features)
- [Why Compile-Time Reflection?](#why-compile-time-reflection) 📖 [Full Article](docs/WHY_REFLECTION.md)
- [Performance](#performance)
- [Positioning](#positioning)
  - [No Extra "Mapping" Layer](#no-extra-mapping-and-validation-handwritten-layer-with-same-performance)
  - [Embedded-Friendliness](#embedded-friendliness)
  - [C Interoperability](#c-interoperability)
- [Compile-Time Schema with Runtime Validation](#one-more-thing-compile-time-schema-runtime-validation-in-the-same-parsing-single-pass)
  - [Supported Options](#supported-options-include)
- [Limitations](#limitations)
- [Benchmarks](#benchmarks)

## Installation

JsonFusion is a **header-only library**. Simply copy the headers to your project's include directory:

```bash
# Copy to your project
cp -r include/JsonFusion /path/to/your/project/include/
cp -r include/pfr* /path/to/your/project/include/
```

Or if you're installing system-wide:

```bash
# System-wide installation (Linux/macOS)
sudo cp -r include/JsonFusion /usr/local/include/
sudo cp -r include/pfr* /usr/local/include/
```

### Using in Your Code

```cpp
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>

// That's it! No linking required.
```

### CMake Integration

If using CMake, add the include directory:

```cmake
include_directories(/path/to/JsonFusion/include)
```

### Requirements

- **C++20** or later
- Compiler: GCC 10+, Clang 13+, MSVC 2019+
- **Boost.PFR** (bundled, no separate installation needed)

## Main features

- Validation of JSON shape and structure, field types compatibility, all done in the same single parsing pass
- No macros, no codegen, no registration – relies on PFR and aggregates
- Header-only
- Works with deeply nested structs, arrays, strings, and arithmetic types out of the box
- Default values via usual C++ defaults
- No data-driven recursion in the parser: recursion depth is bounded by your C++ type nesting, not by JSON depth. With only fixed-size containers, there is no unbounded stack growth.

## Why Compile-Time Reflection?

JsonFusion leverages **compile-time reflection** through Boost.PFR, enabling the compiler to know everything about your types before runtime. This isn't a hack – **C++26 (late 2025) is standardizing native reflection**, making this approach the future of C++.

### Key Benefits

- **Everything known at compile time** – memory layout, field types, sizes all determined before first run
- **Zero runtime overhead** – no dynamic dispatch, no type lookups, no metadata tables
- **Aggressive optimization** – compiler can inline, eliminate dead code, constant-fold everything
- **Predictable resources** – critical for embedded: stack usage calculable, no malloc surprises
- **Less code, more correctness** – your struct IS the schema, no boilerplate duplication

```cpp
Parse(config, json);  // The type tells the compiler how to parse. That's it.
```

**Why this matters for embedded**: With fixed-size containers (`std::array`) + compile-time reflection, you get deterministic timing, zero heap fragmentation, and memory requirements known at link time.

📖 **[Read the full philosophy →](docs/WHY_REFLECTION.md)** – includes C++26 context, detailed technical benefits, and embedded systems discussion.

## Performance

### Competitive with hand-written RapidJSON code
Benchmarks on realistic configs show JsonFusion within the same range as RapidJSON + manual mapping, and sometimes faster, while avoiding the DOM and hand-written conversion code.

## Positioning

### No extra “mapping” and validation handwritten layer, with same performance

Traditional setups use a fast JSON parser (RapidJSON, simdjson, etc.) and then a second layer of hand-written mapping into your C++ types. JsonFusion fuses parsing, mapping and validation into a single pass, so it behaves like a thin, typed layer on top of a fast parser — without the manual glue code.

### Embedded-friendliness

No data-driven recursion in the parser core — depth is bounded by user-defined types and explicit.

No dynamic allocations inside the library. You choose your storage: `std::array`, fixed-size char buffers, or dynamic containers.

Works with forward-only input iterators; you can parse from a stream or byte-by-byte without building a big buffer first.

Plays well with -fno-exceptions / -fno-rtti style builds

### C Interoperability

Add JSON parsing to legacy C codebases **without modifying your C headers**. JsonFusion works with plain C structs through a simple C++ wrapper:

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

Perfect for:
- **Legacy firmware** - Add JSON without touching existing C code
- **FFI boundaries** - Python/Rust → C with typed JSON
- **Embedded C projects** - Modern JSON parsing with zero C code changes

**Note:** C arrays (`int arr[10]`) aren't supported due to PFR limitations, but primitives and nested structs work perfectly. See `examples/c_interop/` for a complete working example.

## One more thing: compile-time schema, runtime validation in the same parsing single pass

Turn your C++ structs into a static schema by annotating fields:
```cpp
struct Motor {
    Annotated<int,
              key<"id">,
              range<1, 8>>                          id;

    Annotated<std::string,
              key<"name">,
              min_length<1>, max_length<32>>        name;

    Annotated<std::array<float, 3>,
              key<"position">,
              max_items<3>>                         position;
};
```

### Supported options include:

- `key<"...">` – JSON field name (decouples schema from C++ member name)
- `range<min,max>` – numeric range checks
- `min_length<N>` / `max_length<N>` – string length
- `min_items<N>` / `max_items<N>` – array size
- `not_required` – optional field without having to use std::optional<T>
- `not_json` – internal/derived fields that are invisible to JSON
- `allow_excess_fields` – per-object policy for unknown JSON fields 
- `as_array` – array destructuring for objects (treat struct as JSON tuple)

```cpp
struct Point {
    float x;
    float y;
    float z;
};
std::list<Annotated<Point, as_array>> ob;
Parse(ob,  std::string_view(R"(
[
    [1, 2, 3],
    [5, 6, 7],
    [8, 9, 10]
]
)"));
```
            

## Limitations

- Designed for C++20 aggregates (POD-like structs). Classes with custom constructors, virtual methods, etc. are not automatically reflectable
- Relies on PFR; a few exotic compilers/ABIs may not be supported.
- This is not a JSON DOM library. It shines when you have a known schema and want to map JSON directly into C++ types; if you need a generic tree & ad-hoc editing, use it alongside a DOM library.

## Benchmarks

Input data, some hardware config
    
    {
        "app_name": "StaticBenchApp",
        "version_major": 1,
        "version_minor": 42,

        "network": {
            "name": "eth0",
            "address": "192.168.0.10/24",
            "port": 5020,
            "enabled": true
        },

        "controller": {
            "name": "main_controller",
            "loop_hz": 1000,
            "motors": [
            {
                "id": 1,
                "name": "X1",
                "position": [1.0, 2.0, 3.0],
                "vel_limits": [10.0, 10.0, 10.0],
                "inverted": false
            },
            {
                "id": 2,
                "name": "Y1",
                "position": [4.0, 5.0, 6.0],
                "vel_limits": [9.5, 9.5, 9.5],
                "inverted": true
            },
            {
                "id": 3,
                "name": "Z1",
                "position": [7.0, 8.0, 9.0],
                "vel_limits": [8.5, 8.5, 8.5],
                "inverted": false
            },
            {
                "id": 4,
                "name": "W1",
                "position": [10.0, 11.0, 12.0],
                "vel_limits": [7.5, 7.5, 7.5],
                "inverted": true
            }
            ],
            "sensors": [
            {
                "type": "imu",
                "model": "IMU-9000",
                "range_min": -3.14,
                "range_max": 3.14,
                "active": true
            },
            {
                "type": "lidar",
                "model": "LIDAR-X20",
                "range_min": 0.2,
                "range_max": 30.0,
                "active": true
            },
            {
                "type": "encoder",
                "model": "ENC-5000",
                "range_min": 0.0,
                "range_max": 1000.0,
                "active": false
            },
            {
                "type": "temp",
                "model": "TMP-100",
                "range_min": -40.0,
                "range_max": 125.0,
                "active": true
            }
            ]
        },

        "logging": {
            "enabled": true,
            "path": "/var/log/static_bench",
            "max_files": 8
        }
    }
    

And such models:

*Note, that first one uses static containers (std::array) and second one uses dynamic containers(`std::vector`, `std::string` and `std::list`).
Both declare some typechecks on several fields.*

```cpp
namespace static_model {

//std::array<char, N> is treted like null-terminated C string
using SmallStr  = std::array<char, 16>;
using MediumStr = std::array<char, 32>;
using LargeStr  = std::array<char, 64>;

// ---------- Sub-structures ----------

// Network interface configuration
struct NetworkStatic {
    SmallStr   name;
    LargeStr   address; // e.g. "192.168.0.1/24"
    int        port;
    bool       enabled;
};

// Motor channel configuration
struct MotorStatic {
    int         id;
    SmallStr       name;
    Annotated<std::array<
        Annotated<float, range<-1000, 1000>>,3>, min_items<3>>      position;        // [x,y,z]
    Annotated<std::array<
        Annotated<float, range<-1000, 1000>>,3>, min_items<3>>    vel_limits;      // [vx,vy,vz]
    bool     inverted;
};

// Sensor configuration
struct SensorStatic {
    SmallStr    type;    
    MediumStr   model;
    Annotated<float, range<-100, 100000> >  range_min;
    float      range_max;
    bool       active;
};

// Controller-level configuration
struct ControllerStatic {
    MediumStr                             name;
    Annotated<int, range<200, 10000>>     loop_hz;
    Annotated<std::array<MotorStatic, 4>, min_items<1>>    motors;
    std::array<SensorStatic, 4>  sensors;
};

// Logging configuration
struct LoggingStatic {
    bool        enabled;
    LargeStr    path;
    int         max_files;
};

// Top-level config
struct StaticComplexConfig {
    MediumStr         app_name;
    int               version_major;
    int               version_minor;
    NetworkStatic     network;
    ControllerStatic  controller;
    LoggingStatic     logging;
};
}

namespace dynamic_model {

struct NetworkDynamic {
    string    name;
    string  address; // e.g. "192.168.0.1/24"
    int     port;
    bool    enabled;
};

// Motor channel configuration
struct MotorDynamic {
    int             id;
    string          name;
    Annotated<vector<
            Annotated<float, range<-1000, 1000>>
        >,  max_items<3>, min_items<3>>      position;  
    Annotated<vector<
            Annotated<float, range<-1000, 1000>>
        >,  max_items<3>, min_items<3>>    vel_limits; 
    bool        inverted;
};

// Sensor configuration
struct SensorDynamic {
    string  type;       // "lidar", "imu", ...
    string  model;
    Annotated<float, range<-100, 100000> >  range_min;
    float   range_max;
    bool    active;
};

// Controller-level configuration
struct ControllerDynamic {
    string                     name;
    Annotated<int, range<200, 10000>>     loop_hz;
    Annotated<list<MotorDynamic>,  min_items<1>,  max_items<4>>    motors;
    Annotated<list<SensorDynamic>, max_items<4>>   sensors;
};

// Logging configuration
struct LoggingDynamic {
    bool      enabled;
    string    path;
    int       max_files;
};

// Top-level config
struct DynamicComplexConfig {
    string             app_name;
    int                version_major;
    int                version_minor;
    NetworkDynamic     network;
    ControllerDynamic  controller;
    LoggingDynamic     logging;
};

}
```


This library shows about the same speed as RapidJSON for static containers (sometimes slightly faster) and 25-30% slower for dynamic containers — **but with full mapping and validation in both cases**.

**Important:** RapidJSON below is only parsing into a Document DOM, while JsonFusion parses directly into the final struct with validation.

### RapidJSON DOM parsing

```
rj::Document doc;
doc.Parse(kJsonStatic.data(), kJsonStatic.size());
```
**2.42 - 2.49 µs**

### JsonFusion with static containers + validation

```
StaticComplexConfig cfg;
Parse(cfg, kJsonStatic.data(), kJsonStatic.size());
```
**2.40 - 2.44 µs** ✨ (faster than RapidJSON DOM, with validation!)

### JsonFusion with dynamic containers + validation

```
DynamicComplexConfig cfg;
Parse(cfg, kJsonStatic.data(), kJsonStatic.size());
```
**2.97 - 3.07 µs** (still competitive!)

*GCC 13.2, arm64 Apple M1 Max, Ubuntu Linux on Parallels VM*
