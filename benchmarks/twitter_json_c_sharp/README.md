# C# Twitter.json Benchmark

Cross-language performance comparison: C# System.Text.Json vs C++ JSON libraries (JsonFusion, RapidJSON, reflect-cpp, Glaze).

## Purpose

Provides a fair comparison between:
- **C#**: Managed runtime, garbage collection, JIT compilation
- **C++**: Manual memory management, compile-time optimization, zero-cost abstractions

Same machine, same data, equivalent models—shows real-world performance differences.

## Model Structure

The C# model in `TwitterModel.cs` is a direct equivalent of the C++ `twitter_model_generic.hpp`:
- `optional<T>` → `T?` (nullable reference/value types)
- `vector<T>` → `List<T>`
- `unique_ptr<T>` → `T?` (C# has garbage collection)
- Field names use `[JsonPropertyName]` attributes for JSON mapping

## Building

Requires **.NET 8.0** or later:

```bash
cd benchmarks/twitter_json_c_sharp
dotnet build -c Release
```

## Running

```bash
dotnet run -c Release -- /path/to/twitter.json
```

Example:
```bash
dotnet run -c Release -- ../twitter.json
```

## What Gets Measured

1. **JIT Warmup**: 1000 iterations to ensure C# JIT has fully optimized the code
2. **Benchmark**: 10,000 iterations (same as C++ benchmarks)
3. **Full workflow**: JSON string → Deserialize → Populate typed C# objects

## Expected Output

```
=== C# System.Text.Json Benchmark ===

Warming up JIT (1000 iterations)...
Warmup complete.

System.Text.Json parsing + populating                            XXX.XX µs/iter  (10000 iterations)

Verification: Parsed 100 statuses
First status text: ...

Benchmark complete.
```

## Notes

- **JIT warmup is critical**: C# performance improves significantly after JIT compilation
- **GC pressure**: C# allocates on managed heap; GC runs may affect timing
- **Fair comparison**: System.Text.Json is Microsoft's optimized, modern JSON library
- **Release mode required**: Debug builds are orders of magnitude slower

## Comparison Context

For the same twitter.json file on the same machine:

**C++ (no GC, manual memory):**
- RapidJSON + manual: ~1466 µs
- reflect-cpp: ~1190 µs
- JsonFusion + yyjson: ~776 µs
- Glaze: ???

**C# (GC, JIT):**
- System.Text.Json: ??? (you'll find out!)

The comparison shows whether C++'s zero-cost abstractions and manual memory management provide measurable advantages over C#'s productivity-focused runtime for JSON parsing workloads.

