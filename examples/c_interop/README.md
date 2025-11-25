# C Interoperability Example

This example demonstrates using JsonFusion with plain C structs, enabling JSON parsing/serialization for legacy C code without modifying the C headers.

## Files

- `structures.h` - Plain C header with struct definitions
- `parser.cpp` - C++ wrapper that exposes C API for JSON operations
- `main.c` - Pure C code that uses the wrapper

## Use Case

Perfect for:
- Adding JSON support to legacy C codebases
- Embedded systems with C firmware
- FFI boundaries (Python/Rust → C)
- Keeping C headers pure while using modern C++ JSON library

## Building

Using Make:
```bash
make
make test
```

Or manually:
```bash
# Compile C++ wrapper
g++ -std=c++23 -I../../include -c parser.cpp -o parser.o

# Compile C main
gcc -std=c99 -c main.c -o main.o

# Link together
g++ -std=c++23 -I../../include -o c_interop_test main.o parser.o

# Run
./c_interop_test
```

## Key Points

1. **No C code changes** - `structures.h` is pure C
2. **C++ stays isolated** - Only in `parser.cpp`
3. **Standard C API** - Simple function calls, error codes
4. **Both parsing and serialization** - Full round-trip support

## Important Limitations with C Structs

### ✅ What Works
- Primitive types (`int`, `float`, `double`, `bool`/`int`)
- Nested structs
- Multiple levels of nesting

### ❌ What Does NOT Work
- **C arrays** (`int arr[10]`, `float data[3]`) - PFR treats each element as a separate field
- **Pointers** (`char*`, `int*`) - No automatic allocation
- **Flexible array members**
- **Unions** - Not supported by PFR

### 💡 Workaround for Arrays
If you need arrays, use C++ on the parsing side with `std::array`:

```cpp
// In your C++ wrapper struct (not in C header):
struct DeviceConfigCpp {
    int device_id;
    float temperature;
    std::array<float, 4> readings;  // ✅ Works!
};

// Then manually copy to C struct if needed
```

