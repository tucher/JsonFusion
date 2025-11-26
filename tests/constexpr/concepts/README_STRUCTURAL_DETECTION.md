# Structural Detection Tests

## Purpose

The `test_structural_detection.cpp` file provides **comprehensive compile-time verification** that JsonFusion's type classification system correctly identifies each type as belonging to **exactly one primary concept** and **not** any other concept.

## Why This Is Critical

**Mutual Exclusivity**: These tests ensure that if type T matches concept A, it must NOT match concepts B, C, D, etc.

Type misclassification can cause catastrophic failures:
- A map detected as an object → parser expects struct fields instead of key-value pairs
- An array detected as an object → wrong parsing logic applied
- An object detected as a map → unexpected behavior

These tests run **first** in the test suite because if type detection is broken, all other tests are meaningless.

## Test Coverage

### 1. **JsonBool** (Section 1)
- ✅ `bool` IS `JsonBool`
- ❌ `bool` is NOT `JsonNumber`, `JsonString`, `JsonObject`, `JsonParsableArray`, `JsonParsableMap`
- ❌ Other types are NOT `JsonBool`

### 2. **JsonNumber** (Section 2)
- ✅ All integral types (`int`, `uint32_t`, etc.) ARE `JsonNumber`
- ✅ All floating types (`float`, `double`) ARE `JsonNumber`
- ❌ Numbers are NOT `JsonBool`, `JsonString`, `JsonObject`, `JsonParsableArray`, `JsonParsableMap`
- ❌ Other types are NOT `JsonNumber`

### 3. **JsonString** (Section 3)
- ✅ `std::array<char, N>` IS `JsonString`
- ❌ Strings are NOT `JsonBool`, `JsonNumber`, `JsonObject`, `JsonParsableArray`, `JsonParsableMap`
- ❌ `std::array<int, N>` is NOT `JsonString` (wrong element type)
- ❌ Other types are NOT `JsonString`

### 4. **JsonObject** (Section 4)
- ✅ Aggregate structs ARE `JsonObject`
- ❌ Objects are NOT `JsonBool`, `JsonNumber`, `JsonString`, `JsonParsableArray`, `JsonParsableMap`
- **CRITICAL**: ❌ Maps are NOT `JsonObject`
- **CRITICAL**: ❌ Arrays are NOT `JsonObject`
- ❌ Other types are NOT `JsonObject`

### 5. **JsonParsableArray** (Section 5)
- ✅ `std::array<T, N>` (non-char) ARE `JsonParsableArray`
- ❌ Arrays are NOT `JsonBool`, `JsonNumber`, `JsonString`, `JsonObject`, `JsonParsableMap`
- **CRITICAL**: ❌ Arrays are NOT `JsonObject`
- **CRITICAL**: ❌ Arrays are NOT `JsonParsableMap`
- ❌ `std::array<char, N>` is NOT `JsonParsableArray` (it's `JsonString`)
- ❌ Other types are NOT `JsonParsableArray`

### 6. **JsonParsableMap** (Section 6)
- ✅ Types with `key_type`, `mapped_type`, `try_emplace`, `clear` ARE `JsonParsableMap`
- ❌ Maps are NOT `JsonBool`, `JsonNumber`, `JsonString`, `JsonObject`, `JsonParsableArray`
- **CRITICAL**: ❌ Maps are NOT `JsonObject`
- **CRITICAL**: ❌ Objects are NOT `JsonParsableMap`
- ❌ Map-like types with non-string keys are NOT `JsonParsableMap`
- ❌ Other types are NOT `JsonParsableMap`

### 7. **Streaming Containers** (Section 7)
Tests custom consumer types (like `MapConsumer` from `test_map_streaming.cpp`):

- ✅ `MapConsumer` IS `JsonParsableMap`
- **CRITICAL**: ❌ `MapConsumer` is NOT `JsonObject`
- ✅ `ArrayConsumer` IS `JsonParsableArray`
- **CRITICAL**: ❌ `ArrayConsumer` is NOT `JsonObject` or `JsonParsableMap`

### 8. **Optional/Nullable Types** (Section 8)
- ✅ `std::optional<T>` IS `JsonNullableParsableValue`
- ❌ `std::optional<T>` is NOT `JsonNonNullableParsableValue`
- ✅ Non-optional types ARE `JsonNonNullableParsableValue`
- ❌ Non-optional types are NOT `JsonNullableParsableValue`

### 9. **Classification Matrix** (Section 9)
**Verifies mutual exclusivity** using a helper function:
```cpp
template<typename T>
consteval int count_matching_concepts() {
    int count = 0;
    if constexpr (JsonBool<T>) count++;
    if constexpr (JsonNumber<T>) count++;
    if constexpr (JsonString<T>) count++;
    if constexpr (JsonObject<T>) count++;
    if constexpr (JsonParsableArray<T>) count++;
    if constexpr (JsonParsableMap<T>) count++;
    return count;
}
```

**Assertions:**
- ✅ Each valid type matches **EXACTLY ONE** concept
- ✅ Invalid types (pointers) match **ZERO** concepts

### 10. **Edge Cases** (Section 10)
- Empty structs
- Single-field structs
- "Fake maps" (structs with `key_type`/`mapped_type` but no map interface)
- Zero-sized arrays
- Deeply nested types

### 11. **Annotated Types** (Section 11)
- Annotated types preserve their underlying concept
- `Annotated<int, key<"x">>` IS still `JsonNumber`
- `Annotated<std::array<int, 10>, key<"items">>` IS still `JsonParsableArray`

## Key Insights

### Type Detection Order

The `is_json_object` check explicitly excludes maps and arrays:

```cpp
else if constexpr (MapReadable<U>) {
    return false; // maps are handled separately
}
else if constexpr (MapWritable<U>) {
    return false; // maps are handled separately
}
else if constexpr (ArrayReadable<U>) {
    return false; // arrays are handled separately
}
else if constexpr (ArrayWritable<U>) {
    return false; // arrays are handled separately
}
```

**Detection order (critical):**
1. Primitives (bool, number, string)
2. Arrays → check `ArrayReadable`/`ArrayWritable`
3. Maps → check `MapReadable`/`MapWritable`
4. Objects → aggregate structs (fallback)

This ordering prevents ambiguity: maps and arrays must be detected **before** the generic object check.

## Running the Tests

```bash
# Compile only (no executable needed)
g++ -std=c++23 -I../../include -c test_structural_detection.cpp

# Or run all constexpr tests
cd tests/constexpr
./run_tests.sh
```

All assertions are `static_assert`, so:
- ✅ If compilation succeeds → All tests pass
- ❌ If compilation fails → Test failure with specific error

## Test Philosophy

> "A type should be **exactly what it is**, and **nothing else**."

This test suite embodies defensive programming:
- Test not just **what should match**
- Test also **what should NOT match**
- Verify mutual exclusivity to catch classification bugs early

**These tests are the foundation** - if type detection is broken, the entire system fails.

