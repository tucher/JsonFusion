# I/O Iterator Tests

## Purpose

Demonstrates that JsonFusion works with **true iterators**, not just pointers. The parser can process JSON byte-by-byte through custom input iterators that explicitly forbid pointer arithmetic and random access.

## Key Demonstration

### Custom Iterator: `ByteByByteInputIterator<Container>`

**Capabilities (allowed):**
- ✅ `++it` - Forward iteration only
- ✅ `*it` - Dereference current position
- ✅ `it == other` - Equality comparison
- ✅ Satisfies `std::input_iterator` concept

**Explicitly Deleted (proving they're not needed):**
- ❌ `it + n` - Pointer arithmetic
- ❌ `it - other` - Distance calculation
- ❌ `it[n]` - Random access
- ❌ `it += n` - Arithmetic assignment
- ❌ `it < other` - Ordering comparisons

## What This Proves

1. **True streaming support**: Parser doesn't require contiguous memory or random access
2. **Works with any forward iterator**: Network streams, file I/O, custom protocols
3. **Byte-by-byte processing**: Can feed JSON one character at a time
4. **Compile-time verification**: All tests run in `constexpr` with `static_assert`

## Test Coverage

### Parsing with Custom Iterators (8 tests)
- ✅ Primitive types (`int`, `bool`)
- ✅ Nested structures
- ✅ Arrays (`std::vector`, `std::array`)
- ✅ Optional fields (`std::optional`)
- ✅ Complex nested types (`std::vector<Point>`, `std::optional<std::vector<int>>`)
- ✅ Error detection (type mismatches)
- ✅ Whitespace handling
- ✅ String escapes and Unicode

### Example

```cpp
// Custom iterator with deleted pointer operations:
ByteByByteInputIterator<std::string> begin(json, 0);
ByteByByteInputIterator<std::string> end(json, json.size());

// This works - parser only needs forward iteration:
auto result = JsonFusion::Parse(obj, begin, end);  // ✅
```

## Real-World Applications

This capability enables:
- **Network parsing**: Read JSON from socket byte-by-byte without buffering
- **File streaming**: Parse large files without loading into memory
- **Serial I/O**: Process JSON from UART/SPI on embedded systems
- **Custom protocols**: Wrap any byte source (decompressors, decryptors, etc.)

## File

- `test_io_iterators.cpp` - All tests in single file, 8 static_assert blocks

