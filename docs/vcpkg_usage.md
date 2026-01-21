# vcpkg Usage

JsonFusion is available in the [official vcpkg registry](https://github.com/microsoft/vcpkg/blob/master/ports/jsonfusion/vcpkg.json).

## Quick Start

### Install JsonFusion

```bash
vcpkg install jsonfusion
```

### CMake Integration

In your `CMakeLists.txt`:

```cmake
find_package(jsonfusion CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE jsonfusion::jsonfusion)
target_compile_features(your_target PUBLIC cxx_std_23)
```

### Build with vcpkg Toolchain

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

## Using vcpkg.json (Manifest Mode)

Create `vcpkg.json` in your project root:

```json
{
  "dependencies": [
    "jsonfusion"
  ]
}
```

Then configure and build:

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

Dependencies (including `boost-pfr`) are automatically installed.

## Requirements

- vcpkg: Follow the [vcpkg getting started guide](https://vcpkg.io/en/getting-started.html)
- C++23 compatible compiler (GCC 14+)

## Version

The vcpkg port tracks JsonFusion releases. Check the [vcpkg registry](https://github.com/microsoft/vcpkg/tree/master/ports/jsonfusion) for the current version.
