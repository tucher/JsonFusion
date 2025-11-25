# JsonFusion Examples

This directory contains example programs demonstrating JsonFusion usage.

## Building Examples

### Option 1: Using g++ directly

```bash
cd examples

# Basic usage
g++ -std=c++23 -I../include basic_usage.cpp -o basic_usage
./basic_usage

# Validation example
g++ -std=c++23 -I../include validation_example.cpp -o validation_example
./validation_example
```

### Option 2: Using clang++

```bash
clang++ -std=c++23 -I../include basic_usage.cpp -o basic_usage
./basic_usage
```

### Option 3: Using CMake (from root directory)

```bash
cd ..
mkdir build && cd build
cmake ..
cmake --build .
```

## Examples Overview

### `basic_usage.cpp`
Demonstrates:
- Simple struct parsing
- Nested objects
- Basic error handling

### `validation_example.cpp`
Demonstrates:
- Field annotations with `Annotated<>`
- Range validation
- String length constraints
- Compile-time schema validation with runtime checks

## Quick Test

All examples should compile and run without errors. The validation example intentionally shows both successful parsing and validation failures.

