# Embedded Binary Size Benchmark

Measures JsonFusion's code footprint for embedded ARM Cortex-M7 targets.

## Overview

This benchmark compiles a realistic embedded configuration parser and measures the resulting binary size. The goal is to understand JsonFusion's flash memory requirements for resource-constrained embedded systems.

## Model

The test uses `EmbeddedConfig` - a realistic embedded configuration with:
- **Fixed-size arrays** (static allocation, no heap)
- **Validation constraints** (`range<>`, `min_items<>`)
- **Nested structures** (Network, Controller, Motor, Sensor, Logging)
- **Optional fields** (`std::optional`)
- **~2.5 KB data size** (16 motors, 16 sensors, various strings)

This represents a typical embedded device configuration (motor controllers, IoT devices, industrial systems).

## Target Platform

**ARM Cortex-M7** (common in STM32H7, SAMV7, etc.):
- 32-bit ARM architecture
- Thumb-2 instruction set
- Hardware floating-point unit (FPv5-D16)
- Representative of modern embedded systems

## Prerequisites

Install ARM cross-compilation toolchain:

```bash
# macOS
brew install --cask gcc-arm-embedded

# Ubuntu/Debian
sudo apt-get install gcc-arm-none-eabi

# Verify installation
arm-none-eabi-gcc --version
```

## Building

```bash
./build.sh
```

This builds **4 configurations**:
1. **size_optimized** (`-Os -flto`) - Minimize code size
2. **speed_optimized** (`-O2 -flto`) - Balance size/speed
3. **aggressive_opt** (`-O3 -flto`) - Maximum speed
4. **minimal_size** (`-Os -flto -fno-inline`) - Absolute smallest

## Results

Each build outputs:
- **Code size** (`.text` section) - Flash memory usage
- **Const data** (`.rodata` section) - Validation tables, literals
- **Initialized data** (`.data` section) - Should be minimal
- **Uninitialized data** (`.bss` section) - Global config (~2.5 KB)
- **Top symbols** - Largest functions by size

### Expected Results

Typical code sizes for this model:

| Configuration | Approximate Size | Notes |
|---------------|-----------------|-------|
| `-Os` (size) | ~2-5 KB | Minimal flash usage |
| `-O2` (balanced) | ~3-7 KB | More inlining |
| `-O3` (speed) | ~4-8 KB | Aggressive inlining |

**Key factors:**
- Template instantiation for each field type
- Validation logic (range checks, bounds)
- String parsing optimizations
- LTO effectiveness

## Analysis Commands

### Size breakdown:
```bash
arm-none-eabi-size -A parse_config_size_optimized.o
```

### Symbol sizes (largest first):
```bash
arm-none-eabi-nm --size-sort --reverse-sort parse_config_size_optimized.o
```

### Disassembly (inspect generated code):
```bash
arm-none-eabi-objdump -d parse_config_size_optimized.o | less
```

### Section headers:
```bash
arm-none-eabi-objdump -h parse_config_size_optimized.o
```

## Comparisons

To compare with alternatives:

### RapidJSON + Manual Parsing
Create `parse_config_rapidjson.cpp` with hand-written DOM traversal (200+ lines).

### Hand-Written Parser
Create minimal hand-written parser (no validation, error-prone).

### Other Embedded Libraries
- ArduinoJson
- jsmn (minimal tokenizer)
- cJSON

Expected results:
- **JsonFusion**: Best size/features trade-off
- **Hand-written**: Smallest but error-prone, no validation
- **RapidJSON**: Larger due to DOM overhead
- **Minimal parsers**: Smaller but no type safety or validation

## Optimization Tips

To minimize code size:

### 1. Use `-Os` and LTO
```bash
-Os -flto
```

### 2. Disable exceptions and RTTI
```bash
-fno-exceptions -fno-rtti
```

### 3. Enable dead code elimination
```bash
-ffunction-sections -fdata-sections
-Wl,--gc-sections
```


## Notes

- This measures **just the parser code** (no main, no runtime, no libc)
- Real application will add: startup code, interrupt vectors, drivers, etc.
- LTO is **critical** for embedded - enables aggressive dead code elimination
- Consider using `JSONFUSION_USE_FAST_FLOAT=0` for even smaller binaries (uses standard `strtod`)

## Future Work

- [ ] Add RapidJSON comparison
- [ ] Add hand-written parser baseline
- [ ] Test with different models (simpler/more complex)
- [ ] Measure with actual embedded linker script
- [ ] Test on other targets (Cortex-M0+, M4, RISC-V)

