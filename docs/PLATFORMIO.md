# PlatformIO Integration Guide

## Requirements

- **GCC 14 or newer** (most PlatformIO platforms ship with older toolchains)
- **C++23** is mandatory
- Must explicitly remove older standard flags with `build_unflags`

## Complete Example

### platformio.ini

```ini
[env:nucleo_f767zi]
platform = ststm32
board = nucleo_f767zi
framework = arduino

platform_packages = 
    toolchain-gccarmnoneeabi@1.140201.0

lib_deps = 
    https://github.com/tucher/JsonFusion.git

build_unflags = 
    -std=gnu++14
    -std=gnu++17

build_flags = 
    -std=c++23
    -D_POSIX_C_SOURCE=200809L
```

### src/main.cpp

```cpp
#include <JsonFusion/parser.hpp>

struct Config {
    std::string app_name;
    int version;
    bool debug_mode;
    
    struct Server {
        std::string host;
        int port;
    };
    Server server;
};

void setup() {
    const char* json = R"({
        "app_name": "MyApp",
        "version": 1,
        "debug_mode": true,
        "server": {
            "host": "localhost",
            "port": 8080
        }
    })";
    Config config;
    bool result = JsonFusion::Parse(config, std::string_view(json));
}
```

## Framework Notes

### Arduino Framework
- Requires `-D_POSIX_C_SOURCE=200809L` to fix `vdprintf` errors in `Print::printf()`

### STM32Cube Framework
- No additional flags needed
- Better suited for production embedded systems

```ini
framework = stm32cube  ; Instead of arduino
```

## More Information

- [Main README](README.md) - Full feature documentation
- [Annotations Reference](docs/ANNOTATIONS_REFERENCE.md) - Validation and options
- [Embedded Benchmarks](benchmarks/embedded/code_size/) - Code size analysis
- [Examples](examples/) - Complete working examples

