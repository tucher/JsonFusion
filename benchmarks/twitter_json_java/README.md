# Twitter JSON Benchmark - Java DSL-JSON

Cross-language benchmark for `twitter.json` parsing using **DSL-JSON**, a high-performance Java JSON library with compile-time code generation.

## About DSL-JSON

[DSL-JSON](https://github.com/ngs-doo/dsl-json) is a Java library that performs compile-time analysis and code generation for JSON serialization/deserialization. It's comparable to:
- C++ template-based approaches (JsonFusion, Glaze)
- C# source generators (System.Text.Json with `JsonSerializerContext`)
- Faster than reflection-based libraries (Jackson, Gson)

## Prerequisites

- **Java 21** or later (LTS version recommended)
- **Maven 3.6+** for building

### Install Java (macOS)

```bash
brew install openjdk@21
```

### Install Java (Ubuntu)

```bash
sudo apt-get update
sudo apt-get install openjdk-21-jdk maven
```

### Verify Installation

```bash
java -version  # Should show Java 21+
mvn -version   # Should show Maven 3.6+
```

## Building

From this directory:

```bash
# Clean and build (first time or after changes)
mvn clean package

# Just build (incremental)
mvn package
```

This will:
1. Download DSL-JSON and dependencies
2. Run the annotation processor to generate serialization code at compile-time
3. Create an executable JAR: `target/twitter-benchmark-1.0-SNAPSHOT.jar`

**Note:** The first build may take ~30-60 seconds to download dependencies.

## Running

```bash
# Run with path to twitter.json
java -jar target/twitter-benchmark-1.0-SNAPSHOT.jar ../../twitter.json

# Or using Maven
mvn exec:java -Dexec.args="../../twitter.json"
```

### Expected Output

```
=== Twitter.json Parsing Benchmark - Java DSL-JSON ===

Reading file: ../../twitter.json
File size: 631514 bytes

Warming up JIT (1000 iterations)...
Warmup complete.

Running benchmark (10000 iterations)...
Benchmark complete.

=== Results ===
DSL-JSON parsing + populating                                   XXX.XX µs/iter

Total time: XXXX.XX ms for 10000 iterations

Sanity check: Parsed 100 statuses
First status by: <username>
```

## Performance Comparison

This benchmark is directly comparable to:
- **C++**: JsonFusion, Glaze, reflect-cpp, RapidJSON (in `../twitter_json/`)
- **C#**: System.Text.Json (in `../twitter_json_c_sharp/`)

All benchmarks:
- Parse the same `twitter.json` file (~630 KB, deeply nested)
- Use 1000 warmup iterations
- Measure 10,000 parsing iterations
- Report µs/iteration

Expected performance for DSL-JSON: **~700-1200 µs/iter** (competitive with C++ compiled approaches)

## How DSL-JSON Works

1. **Compile-time**: Annotation processor scans `@CompiledJson` classes and generates optimized serialization code
2. **Runtime**: No reflection, no bytecode generation - just direct field access
3. **Result**: Performance approaching hand-written parsers, similar to C++ template metaprogramming

Compare to:
- **Jackson** (reflection): Slower, uses runtime reflection
- **Gson** (reflection): Even slower, more allocation overhead
- **Jsoniter** (bytecode): Fast, but runtime generation
- **DSL-JSON** (compile-time): Fast, zero reflection, Native Image compatible

## Project Structure

```
twitter_json_java/
├── pom.xml                           # Maven build configuration
├── README.md                         # This file
└── src/main/java/com/jsonfusion/benchmark/
    ├── TwitterModel.java             # Data model with @CompiledJson annotations
    └── TwitterBenchmark.java         # Main benchmark runner
```

## Cross-Language Insights

### Model Equivalence

| C++ (JsonFusion) | C# | Java |
|------------------|----|----- |
| `std::optional<T>` | `T?` | `T` (nullable) |
| `std::vector<T>` | `List<T>` | `List<T>` |
| `std::string` | `string` | `String` |
| `A<key<"protected">>` | `[JsonPropertyName("protected")]` | `@JsonAttribute(name = "protected")` |

### Performance Characteristics

- **C++ (Glaze)**: Fastest (~350 µs) - hand-optimized, zero-cost abstractions
- **C++ (JsonFusion + yyjson)**: Very fast (~780 µs) - pluggable backend
- **Java (DSL-JSON)**: Expected ~800-1200 µs - compile-time generation, JIT warmup
- **C# (System.Text.Json)**: ~1500 µs - similar to Java, managed runtime
- **C++ (reflect-cpp)**: ~1200 µs - DOM-based, two-pass parsing

The JVM's mature JIT compiler and aggressive optimizations keep Java competitive with C++ for JSON parsing!

## Troubleshooting

### "Unsupported class file major version"

Your Java version is too old. Install Java 21+:
```bash
brew install openjdk@21  # macOS
sudo apt-get install openjdk-21-jdk  # Ubuntu
```

### "Cannot find symbol: class TwitterData_dslJsonDecoder"

The annotation processor didn't run. Try:
```bash
mvn clean package  # Full rebuild
```

### Maven is slow

Maven caches dependencies. First build is slow, subsequent builds are fast.

## License

Same as JsonFusion (see root LICENSE.md)

