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

### 1. **BoolLike** (Section 1)
- ✅ `bool` IS `BoolLike`
- ❌ `bool` is NOT `NumberLike`, `ParsableStringLike`, `SerializableStringLike`, `ObjectLike`, `ParsableArrayLike`, `ParsableMapLike`
- ❌ Other types are NOT `BoolLike`

### 2. **NumberLike** (Section 2)
- ✅ All integral types (`int`, `uint32_t`, etc.) ARE `NumberLike`
- ✅ All floating types (`float`, `double`) ARE `NumberLike`
- ❌ Numbers are NOT `BoolLike`, `ParsableStringLike`, `SerializableStringLike`, `ObjectLike`, `ParsableArrayLike`, `ParsableMapLike`
- ❌ Other types are NOT `NumberLike`

### 3. **ParsableStringLike / SerializableStringLike** (Section 3)
- ✅ `std::array<char, N>` IS both `ParsableStringLike` and `SerializableStringLike`
- ❌ Strings are NOT `BoolLike`, `NumberLike`, `ObjectLike`, `ParsableArrayLike`, `ParsableMapLike`
- ❌ `std::array<int, N>` is NOT `ParsableStringLike`/`SerializableStringLike` (wrong element type)
- ❌ Other types are NOT `ParsableStringLike`/`SerializableStringLike`

### 4. **ObjectLike** (Section 4)
- ✅ Aggregate structs ARE `ObjectLike`
- ❌ Objects are NOT `BoolLike`, `NumberLike`, `ParsableStringLike`, `SerializableStringLike`, `ParsableArrayLike`, `ParsableMapLike`
- **CRITICAL**: ❌ Maps are NOT `ObjectLike`
- **CRITICAL**: ❌ Arrays are NOT `ObjectLike`
- ❌ Other types are NOT `ObjectLike`

### 5. **ParsableArrayLike** (Section 5)
- ✅ `std::array<T, N>` (non-char) ARE `ParsableArrayLike`
- ❌ Arrays are NOT `BoolLike`, `NumberLike`, `ParsableStringLike`, `SerializableStringLike`, `ObjectLike`, `ParsableMapLike`
- **CRITICAL**: ❌ Arrays are NOT `ObjectLike`
- **CRITICAL**: ❌ Arrays are NOT `ParsableMapLike`
- ❌ `std::array<char, N>` is NOT `ParsableArrayLike` (it's `ParsableStringLike`)
- ❌ Other types are NOT `ParsableArrayLike`

### 6. **ParsableMapLike** (Section 6)
- ✅ Types with `key_type`, `mapped_type`, `try_emplace`, `clear` ARE `ParsableMapLike`
- ❌ Maps are NOT `BoolLike`, `NumberLike`, `ParsableStringLike`, `SerializableStringLike`, `ObjectLike`, `ParsableArrayLike`
- **CRITICAL**: ❌ Maps are NOT `ObjectLike`
- **CRITICAL**: ❌ Objects are NOT `ParsableMapLike`
- ❌ Map-like types with non-string keys are NOT `ParsableMapLike`
- ❌ Other types are NOT `ParsableMapLike`

### 7. **Streaming Containers** (Section 7)
Tests custom consumer types (like `MapConsumer` from `test_map_streaming.cpp`):

- ✅ `MapConsumer` IS `ParsableMapLike`
- **CRITICAL**: ❌ `MapConsumer` is NOT `ObjectLike`
- ✅ `ArrayConsumer` IS `ParsableArrayLike`
- **CRITICAL**: ❌ `ArrayConsumer` is NOT `ObjectLike` or `ParsableMapLike`

### 8. **Optional/Nullable Types** (Section 8)
- ✅ `std::optional<T>` IS `NullableParsableValue`
- ❌ `std::optional<T>` is NOT `NonNullableParsableValue`
- ✅ Non-optional types ARE `NonNullableParsableValue`
- ❌ Non-optional types are NOT `NullableParsableValue`

### 9. **Classification Matrix** (Section 9)
**Verifies mutual exclusivity** using a helper function:
```cpp
template<typename T>
consteval int count_matching_concepts() {
    int count = 0;
    if constexpr (BoolLike<T>) count++;
    if constexpr (NumberLike<T>) count++;
    if constexpr (ParsableStringLike<T>) count++;
    if constexpr (ObjectLike<T>) count++;
    if constexpr (ParsableArrayLike<T>) count++;
    if constexpr (ParsableMapLike<T>) count++;
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
- `Annotated<int, key<"x">>` IS still `NumberLike`
- `Annotated<std::array<int, 10>, key<"items">>` IS still `ParsableArrayLike`

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

All assertions are `static_assert` - if compilation succeeds, all tests pass.

