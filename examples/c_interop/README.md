# C Interoperability Example

This example demonstrates using JsonFusion with plain C structs, enabling JSON parsing/serialization for legacy C code without modifying the C headers.

## Files

- `structures.h` - Plain C header with struct definitions (Motor, MotorSystem)
- `parser.cpp` - C++ wrapper that exposes C API for JSON operations with StructMeta annotations
- `main.c` - Pure C code that uses the wrapper

## Use Case

Perfect for:
- Adding JSON support to legacy C codebases
- Embedded systems with C firmware
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
2. **C++ stays isolated** - Only in `parser.cpp` (with StructMeta annotations)
3. **Standard C API** - Simple function calls, error codes
4. **Both parsing and serialization** - Full round-trip support
5. **C arrays supported** - Using `StructMeta` annotations for C arrays

## Example Structures

### Motor
- `int64_t position[3]` - 3D position array (C array)
- `int active` - Active flag (using int as bool for C compatibility)
- `char name[20]` - Motor name (null-terminated C string)

### MotorSystem
- `Motor primary_motor` - Nested Motor structure
- `Motor motors[5]` - C array of up to 5 motors
- `int motor_count` - Number of active motors (0-5)
- `char system_name[32]` - System name (null-terminated C string)
- `double transform[3][3]` - Generic 3x3 transformation matrix
## Important Notes



## StructMeta Annotations

C arrays in structure require to use `StructMeta` annotations in the C++ wrapper (`parser.cpp`):

```cpp
template<>
struct JsonFusion::StructMeta<Motor> {
    using Fields = StructFields<
        Field<&Motor::position, "position", min_items<3>>,
        Field<&Motor::active, "active">,
        Field<&Motor::name, "name", min_length<1>>
    >;
};
```

This tells JsonFusion how to handle C arrays and apply validation constraints. It is needed because of PFR limitations.
