# JsonFusion Conan Consumer Example

This directory demonstrates how to use JsonFusion as a Conan dependency in your project.

## Prerequisites

1. Conan 2.x installed: `pip install conan`
2. C++23 compatible compiler (GCC 14+, Clang 16+, or MSVC 2022+)
3. CMake 3.15 or higher

## Building the Example

### Step 1: Ensure JsonFusion is in your local Conan cache

From the JsonFusion root directory (parent of this example):

```bash
conan create . --build=missing
```

### Step 2: Install dependencies for this example

From this directory (`conan_example/`):

```bash
conan install . --output-folder=build --build=missing
```

If you're using a custom profile (e.g., GCC-14):

```bash
conan install . --output-folder=build --profile=gcc14 --build=missing
```

### Step 3: Configure CMake

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
```

### Step 4: Build and run

```bash
cmake --build build
./build/consumer_app
```

## Expected Output

You should see:
- Successful parsing of sensor configuration
- Display of parsed data
- Modification and re-serialization
- Validation error demonstration

## Project Structure

- `conanfile.txt` - Declares JsonFusion as a dependency
- `CMakeLists.txt` - CMake configuration with `find_package(JsonFusion)`
- `main.cpp` - Example application using JsonFusion
- `README.md` - This file

## Using in Your Own Project

To use JsonFusion in your own project, simply:

1. Copy `conanfile.txt` to your project
2. In your `CMakeLists.txt`, add:
   ```cmake
   find_package(JsonFusion REQUIRED CONFIG)
   target_link_libraries(your_target JsonFusion::JsonFusion)
   target_compile_features(your_target PUBLIC cxx_std_23)
   ```
3. Install dependencies: `conan install . --output-folder=build --build=missing`
4. Build with the Conan toolchain: `cmake .. -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake`

## Documentation

For more details, see `../docs/conan_usage.md`
