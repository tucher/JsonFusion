# JSONReflection, yet another JSON library

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
*No macros, no registration, no JSON DOM, no custom objects*


## Main features

- Validation of JSON shape and structure, field types compatibility, all done in the same single parsing pass
- No macros, no codegen, no registration – relies on PFR and aggregates
- Header-only
- Works with deeply nested structs, arrays, strings, and arithmetic types out of the box
- Custom mappers (TODO)
- Raw JSON extraction/insertion for subparsing/subserializing (TODO)
- Default values via usual C++ defaults
- No data-driven recursion in the parser: recursion depth is bounded by your C++ type nesting, not by JSON depth. With only fixed-size containers, there is no unbounded stack growth.


## Performance

### Competitive with hand-written RapidJSON code
Benchmarks on realistic configs show JSONReflection within the same range as RapidJSON + manual mapping, and sometimes faster, while avoiding the DOM and hand-written conversion code.

## Positioning

### No extra “mapping” and validation handwritten layer, with same performance

Traditional setups use a fast JSON parser (RapidJSON, simdjson, etc.) and then a second layer of hand-written mapping into your C++ types. JSONReflection fuses parsing, mapping and validation into a single pass, so it behaves like a thin, typed layer on top of a fast parser — without the manual glue code.

### Embedded-friendliness

No recursion in the parser core — depth is bounded and explicit.

No dynamic allocations inside the library. You choose your storage: `std::array`, fixed-size char buffers, or dynamic containers.

Works with forward-only input iterators; you can parse from a stream or byte-by-byte without building a big buffer first.

Plays well with -fno-exceptions / -fno-rtti style builds

## One more thing: compile time schema, runtime validation in the same parsing single pass

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
- `allow_excess_fields` – per-object policy for unknown JSON fields (only with this one objects will tolerate excess fields and require skipping machinery with compile-time limited stack)
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
            

## *Limitations

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
Note, that first one uses static containers only and second one uses dynamic containers.
Both declares some typechecks on several fields.

```cpp
namespace static_model {

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


This library shows about the same speed as Rapid JSON for static containers(sometimes slightly faster) and 25-30% slower for dynamic containers.

But with full mapping and user-defined validation in both cases


```
rj::Document doc;
doc.Parse(kJsonStatic.data(), kJsonStatic.size());
```
2.42 - 2.49 us

*RapidJSON is only parsing into Document while JSONReflection parses and fills the struct, doing validations on process*

```
StaticComplexConfig cfg;
Parse(cfg, kJsonStatic.data(), kJsonStatic.size());
```
2.40 - 2.44 us

```
DynamicComplexConfig cfg;
Parse(cfg, kJsonStatic.data(), kJsonStatic.size());
```
2.97 - 3.07 us

GCC 13.2, arm64 Apple M1 Max, Ubuntu linux on Parallels VM