# Why Compile-Time Reflection?

JsonFusion is built on compile-time reflection (via Boost.PFR), and this isn't a temporary hack – **it's the future of C++**.

## C++26 and Beyond: Reflection Becomes Standard

**C++26 (finalized late 2025)** introduces:
- **Native compile-time reflection** ([P2996](https://wg21.link/p2996)) – inspect types, members, and metadata at compile time without library tricks
- **Variadic structured bindings** ([P1061R10](https://isocpp.org/files/papers/P1061R10.html)) – structured bindings can now introduce parameter packs, making aggregate decomposition significantly more powerful

These features will:
1. Make libraries like PFR much cleaner (and eventually unnecessary as reflection becomes a language feature)
2. Enable even better compiler optimizations
3. Standardize what JsonFusion already demonstrates: compile-time introspection is **the right approach**

### What This Means Technically

Both C++26 reflection and variadic structured bindings solve the same fundamental problem: **how to count struct fields and access their names/types at compile time**. That's it. Everything else is built on top.

```cpp
struct Config { int id; float threshold; SensorData sensor; };

// What we need to know at compile time:
// 1. How many fields? (3)
// 2. What are their types? (int, float, SensorData)
// 3. What are their names? ("id", "threshold", "sensor")
```

**Today**: Boost.PFR achieves this through clever template metaprogramming and aggregate initialization tricks.

**C++26**: The language provides direct syntax for this. PFR itself will adopt variadic structured bindings ([P1061R10](https://isocpp.org/files/papers/P1061R10.html)) to simplify its implementation – in fact, newer versions may already use it when available.

**For JsonFusion**: These are pure implementation details. Whether we count fields using PFR's current tricks, PFR with variadic bindings, or native C++26 reflection – the user's code doesn't change. The `Parse()` call works the same way:

```cpp
Parse(config, json);  // Works today with PFR, will work with C++26 reflection
```

The philosophy is constant: **let the compiler know your types at compile time, then leverage that knowledge for zero-cost parsing**.

## Why Compile-Time > Runtime (Especially for Embedded)

Compile-time reflection isn't just "faster" – it's a **fundamentally different paradigm**:

### **Everything Known Before First Run**

```cpp
struct Config { int id; float threshold; SensorData sensor; };
// At compile time, the compiler knows:
// - Exact size: sizeof(Config)
// - Memory layout: offsetof each field
// - Field count: 3
// - Field types: int, float, SensorData
// - Alignment requirements
```

No runtime discovery. No dynamic allocation for metadata. No type introspection overhead.

### **Compiler Can Optimize Aggressively**

- **Dead code elimination**: If you only parse `id` and `threshold`, the compiler can remove unused `sensor` parsing code
- **Constant folding**: Field names, offsets, sizes all fold into constants
- **Inlining**: Entire parsing path can inline into a tight loop
- **Zero-cost abstractions**: Generic code compiles down to hand-written performance

### **Predictable Memory and Stack**

For embedded/real-time systems, this is critical:

```cpp
std::array<Motor, 4> motors;  // Stack allocation, known at compile time
// vs
std::vector<Motor> motors;    // Heap allocation, runtime dependent
```

With fixed-size containers + compile-time reflection:
- **Stack usage**: Calculable before deployment
- **No heap fragmentation**: Everything can be stack or static
- **No malloc failures**: Critical for safety-critical systems
- **Deterministic timing**: No allocation pauses

### **Less Code, More Correctness**

Compare these approaches:

**Traditional (runtime reflection / manual):**
```cpp
// 100+ lines of boilerplate per struct
void ParseConfig(Config& cfg, const rapidjson::Value& v) {
    if (v.HasMember("id") && v["id"].IsInt())
        cfg.id = v["id"].GetInt();
    // ... repeat for every field, every struct
}
```

**JsonFusion (compile-time reflection):**
```cpp
Parse(config, json);  // That's it. Type info comes from the struct itself.
```

The struct **is** the schema. No duplication, no drift between definition and parsing code.

## The Embedded Philosophy

In resource-constrained environments, you want:
1. **Zero runtime overhead** – all introspection at compile time
2. **Predictable resource usage** – memory known statically  
3. **Small binaries** – no reflection metadata tables
4. **Compiler as tool** – leverage optimization, not fight it

Compile-time reflection delivers all of this. JsonFusion proves it works **today**, and C++26 will make it the standard way to write C++.

## Practical Benefits

### For Embedded Systems
- Parse JSON configs on microcontrollers with KB of RAM
- Use `std::array<char, N>` for strings – no malloc
- All memory requirements known at link time
- Perfect for `-fno-exceptions` builds

### For Safety-Critical Systems
- Static analysis tools can verify all code paths
- No hidden allocations or exceptions
- Deterministic behavior
- MISRA-C++ compatible patterns

### For High-Performance Systems
- Parsing speed competitive with hand-written code
- Zero virtual calls, zero dynamic dispatch
- Compiler can vectorize loops
- Cache-friendly data layout

## Trade-offs and Limitations

While compile-time reflection offers significant benefits, it's not a silver bullet. Be realistic about the costs:

### Template Bloat is Real

Heavy template metaprogramming can lead to:
- **Longer compile times** – Complex template instantiations aren't free
- **Larger binaries** – Each instantiation generates code; without careful design, binary size grows
- **Debug build performance** – Unoptimized template code can be much slower than hand-written equivalent

For extremely resource-constrained environments (8-bit microcontrollers, bootloaders, hard real-time systems), this overhead may be unacceptable.

### Not All Environments Support Modern C++

Some embedded/safety-critical contexts restrict or prohibit:
- **C++23 features** – Many embedded toolchains lag years behind standard
- **Standard library** – Freestanding implementations may not have `<tuple>`, `<array>`, etc.

If you're targeting bare-metal ARM Cortex-M0 with 2KB RAM and a C89 compiler, hand-written C parsing code might be your only option.

### Compiler Quality Varies

The promise of "zero-cost abstractions" depends on optimizer quality:
- **Release builds**: Modern GCC/Clang usually deliver on this promise
- **Older compilers**: May generate poor code from complex templates
- **Exotic platforms**: Custom toolchains often have weaker optimizers

### Debugging is Harder

Template errors are notoriously cryptic. When something goes wrong:
- **Error messages** span dozens of lines of template instantiation traces
- **Debugger experience** – Stepping through template code is less intuitive than procedural code
- **Understanding failures** – Requires understanding both your code and the library's metaprogramming

### The Sweet Spot

JsonFusion works best for:
- **Modern embedded** (Cortex-M4+, ESP32, etc.) with decent toolchains
  - *Confirmed compiling*: RP2040 (Raspberry Pi Pico), ARM Cortex-M7, ESP32 with recent GCC versions
  - Note: These are proof-of-concept tests, not production-validated deployments
- **Desktop/server** applications where binary size isn't measured in KB
- **Teams comfortable with c++23** who value reduced boilerplate
- **Projects where type safety** and validation justify the abstraction cost


## Looking Forward

When C++26 reflection becomes standard:
- JsonFusion will adopt it
- Performance will improve further
- The philosophy remains the same


---

**Related Reading:**
- [C++26 Reflection Proposal (P2996)](https://wg21.link/p2996) – Native compile-time reflection
- [Structured Bindings can introduce a Pack (P1061R10)](https://isocpp.org/files/papers/P1061R10.html) – Variadic structured bindings for C++26
- [Boost.PFR Documentation](https://www.boost.org/doc/libs/master/doc/html/boost_pfr.html) – Current reflection library JsonFusion uses

