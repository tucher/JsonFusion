# Floating-Point Correctness Testing Plan

## Context

**Goal:** Validate the in-house FP parser/serializer as the default (`JSONFUSION_FP_BACKEND=0`).

**Backend Architecture:**
```cpp
// include/JsonFusion/fp_to_str.hpp
#ifndef JSONFUSION_FP_BACKEND
#define JSONFUSION_FP_BACKEND 0  // in-house default
#endif

// 0 = In-house (constexpr, zero-deps) ✅ DEFAULT
// 1 = fast_float + simdjson (high-performance, runtime-only)
// 2+ = Custom user backends (plugin interface)
```

**Why In-House as Default:**
1. ✅ Constexpr-friendly (enables compile-time JSON processing)
2. ✅ Zero external dependencies (no libc, no fast_float)
3. ✅ Sufficient accuracy for practical use cases
4. ✅ Performance competitive on real-world workloads (Twitter JSON, synthetic tests)
5. ✅ Extensible (users can plug in custom backends)

**Current State:**
- Basic fuzz testing exists in `tests/fp/main.cpp`:
  - ✅ Parser vs `strtod` (100M iterations)
  - ✅ Self-roundtrip (format → parse)
  - ✅ Serializer vs `snprintf`
  - ✅ Random JSON number generation
  - Current threshold: 1e-20 relative error

## Comprehensive Testing TODO

---

### 1. ULP-Based Correctness Testing [HIGH PRIORITY]

**Goal:** Measure accuracy in Units in Last Place (ULP) rather than relative error.

#### 1.1 ULP Calculation Infrastructure
- [ ] Implement `ulp_distance(double a, double b)` function
  - Compare bit representations directly
  - Handle sign differences correctly
  - Handle zero/denormal edge cases
  - Return maximum ULP distance for special cases (inf, nan)

#### 1.2 Parser ULP Tests
- [ ] Parse → compare against reference (strtod or Ryu/Dragonbox)
- [ ] **Acceptance:** ≤ 1 ULP for all correctly-rounded inputs
- [ ] **Acceptance:** ≤ 2 ULP for 99.99% of inputs (due to tie-breaking)
- [ ] Log worst-case ULP distances in test report

#### 1.3 Serializer ULP Tests
- [ ] Format → parse → compare to original
- [ ] **Acceptance:** `parse(format(x, prec=17)) == x` for all finite doubles
- [ ] Test all precision levels (1-17 decimal digits)
- [ ] Verify shortest representation preference

**Reference:** [ULP Testing Best Practices](https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/)

---

### 2. Boundary Test Sets [HIGH PRIORITY]

**Goal:** Manually crafted decimal strings targeting IEEE-754 tricky points.

#### 2.1 Powers of 2 Boundaries
```cpp
// Numbers near powers of 2 where rounding changes behavior
"1.0000000000000002"  // 1 + ε
"0.9999999999999999"  // 1 - ε  
"2.0000000000000004"  // 2 + ε
"1.9999999999999998"  // 2 - ε
```
- [ ] Test ±1 ULP around all powers of 2: 2^-1074 to 2^1023
- [ ] Test midpoint rounding (tie-breaking)

#### 2.2 Decimal-to-Binary Rounding Corners
```cpp
// Famous difficult cases from David Gay's test suite
"1e23"
"8.988465674311579e+307"
"2.2250738585072014e-308"  // Near min normal
```
- [ ] Import David Gay's test suite (dtoa-test)
- [ ] 1000+ known difficult conversions
- [ ] Verify exact bit-for-bit correctness

**Reference:** [David Gay's dtoa.c test data](http://www.netlib.org/fp/dtoa.c)

#### 2.3 Powers of 10 Stress Test
```cpp
// Exact powers of 10
"1e0", "1e1", "1e2", ..., "1e308"
"1e-0", "1e-1", ..., "1e-323"
```
- [ ] Test all integer powers: 10^-323 to 10^308
- [ ] Test near-powers: `9.999...e307`, `1.000...e308`
- [ ] Verify correct exponent handling

#### 2.4 Subnormal (Denormal) Numbers
```cpp
// Smallest normal: 2.2250738585072014e-308
// Largest subnormal: 2.225073858507201e-308
// Smallest subnormal: 5e-324
```
- [ ] Test all subnormal transitions
- [ ] Test gradual underflow
- [ ] Verify parsing of `5e-324` (smallest positive double)
- [ ] Test serialization doesn't lose subnormals

#### 2.5 Exponent Extremes
```cpp
// Near overflow
"1.7976931348623157e+308"  // DBL_MAX
"1.7976931348623158e+308"  // Would overflow

// Near underflow
"2.2250738585072014e-308"  // DBL_MIN (normal)
"2.2250738585072009e-308"  // Would underflow
```
- [ ] Test around DBL_MAX / DBL_MIN
- [ ] Test overflow → infinity rejection (JSON doesn't support inf)
- [ ] Test underflow → subnormal handling
- [ ] Test underflow → zero handling

#### 2.6 Zero and Negative Zero
```cpp
"0", "-0", "0.0", "-0.0"
"0e0", "0e100", "0e-100"
```
- [ ] Verify both ±0.0 parse correctly
- [ ] Verify sign bit preserved
- [ ] Test serialization of ±0.0

---

### 3. Cross-Library Validation [MEDIUM PRIORITY]

**Goal:** Compare against multiple high-quality reference implementations.

#### 3.1 Ryu Algorithm (Google)
- [ ] Integrate Ryu for serialization cross-check
- [ ] Compare format output (shortest representation)
- [ ] Verify our output parses back to same double
- [ ] **Reference:** https://github.com/ulfjack/ryu

#### 3.2 Dragonbox Algorithm (Junekey Jeon)
- [ ] Integrate Dragonbox for serialization cross-check
- [ ] Known for optimal shortest representation
- [ ] Compare against Dragonbox for 1M random doubles
- [ ] **Reference:** https://github.com/jk-jeon/dragonbox

#### 3.3 double-conversion (Google V8)
- [ ] Use as parser reference (alternative to strtod)
- [ ] Test 1M random JSON numbers
- [ ] Compare ULP distances
- [ ] **Reference:** https://github.com/google/double-conversion

#### 3.4 fast_float (lemire)
- [ ] Compare parsing accuracy (though we know it's not constexpr)
- [ ] Verify we're within 1 ULP of fast_float results
- [ ] Use as performance baseline
- [ ] **Reference:** https://github.com/fastfloat/fast_float

**Acceptance Criteria:**
- ≤ 1 ULP difference from **all** reference libraries for 99.99% of cases
- Document any cases where implementations disagree (rare)

---

### 4. Stress Tests - Specific Patterns [HIGH PRIORITY]

#### 4.1 Rounding Boundary Stress
**Goal:** Test all rounding modes and tie-breaking.

```cpp
// Exact midpoints between representable doubles
"1.0000000000000002220446049250313080847263336181640625"  // Midpoint
"2.0000000000000004440892098500626161694526672363281250"  // Midpoint
```

- [ ] Generate 10,000 exact midpoints across exponent range
- [ ] Verify correct rounding (round-to-even / banker's rounding)
- [ ] Test numbers slightly above/below midpoints
- [ ] Test all 5 IEEE-754 rounding modes (if supported)

**Known Difficult Cases:**
```cpp
// From Ryu paper
"1.0000000000000002"
"1.0000000000000004"  
"9007199254740992"    // 2^53
"9007199254740994"    // 2^53 + 2
```

#### 4.2 Exponent Extremes Stress
**Goal:** Test parser stability near exponent limits.

- [ ] Test `e+308` (near overflow)
  - `1.0e308`, `1.7e308`, `1.79e308`, `1.797e308`, ...
  - Verify correct overflow detection
- [ ] Test `e-324` (near underflow)
  - `5e-324` (smallest), `4e-324` (underflows to 0)
  - Verify gradual underflow
- [ ] Test `e+1000` (way past overflow)
  - Should reject or return error
- [ ] Test `e-1000` (way past underflow)
  - Should underflow to zero
- [ ] Test huge exponents: `1e999999999`
  - Prevent integer overflow in exponent parsing

#### 4.3 Repeated Powers of 10 Stress
**Goal:** Verify decimal-to-binary conversion accuracy.

```cpp
// Multiples of powers of 10
"1.1e1", "1.1e10", "1.1e100", "1.1e200"
"3.33e-1", "3.33e-10", "3.33e-100", "3.33e-200"
"9.99e2", "9.99e20", "9.99e200"
```

- [ ] Test 1000+ combinations: `[1-9].[1-9]e[sign][0-308]`
- [ ] Compare to reference implementations
- [ ] Look for patterns in ULP errors across exponent ranges
- [ ] Document any systematic bias

#### 4.4 Subnormal Stress Test
**Goal:** Verify correct handling of denormalized numbers.

```cpp
// Subnormal boundaries
"2.2250738585072014e-308"  // DBL_MIN (smallest normal)
"2.2250738585072009e-308"  // Largest subnormal
"1.0e-323"                  // Subnormal
"5.0e-324"                  // Smallest positive
"4.9e-324"                  // Rounds to smallest
"2.5e-324"                  // Midpoint (rounds to 0 or smallest)
```

- [ ] Test all 2^52 subnormal values (may be impractical - sample)
- [ ] Test transitions: normal → subnormal → zero
- [ ] Test parsing subnormals from JSON
- [ ] Test serializing subnormals to JSON
- [ ] Verify no precision loss in roundtrip

**Special Cases:**
- [ ] Parse `5e-324` → 0x0000000000000001 (smallest)
- [ ] Parse `2.5e-324` → 0 or 0x1 (midpoint tie-break)
- [ ] Serialize 0x0000000000000001 → parses back correctly

---

### 5. JSON-Specific Edge Cases [MEDIUM PRIORITY]

#### 5.1 Invalid JSON Number Formats
**Goal:** Verify we reject malformed numbers per RFC 8259.

```cpp
// Invalid - leading +
"+42", "+3.14", "+1e10"

// Invalid - leading zeros
"00", "01", "0123", "00.5"

// Invalid - trailing/leading decimal
"42.", ".42"

// Invalid - missing exponent digits
"1e", "1e+", "1e-"

// Invalid - multiple decimals
"1.2.3"

// Invalid - multiple exponents  
"1e2e3"
```

- [ ] Verify parser rejects all invalid formats
- [ ] Return clear error codes/messages
- [ ] Fuzz with malformed JSON numbers

#### 5.2 Valid JSON Number Edge Cases
```cpp
// Valid per RFC 8259
"0", "0.0", "0e0", "0e100"
"-0", "-0.0"
"1E10", "1e10" (both 'E' and 'e')
"1e+10", "1e-10" (explicit sign)
```

- [ ] Verify all valid forms accepted
- [ ] Test case sensitivity (E vs e)
- [ ] Test optional '+' in exponent

---

### 6. Precision and Rounding Tests [MEDIUM PRIORITY]

#### 6.1 Serialization Precision Levels
**Goal:** Verify `float_decimals<N>` produces correct precision.

```cpp
// Test value: π = 3.14159265358979323846...
serialize(π, decimals=1)  → "3.1"
serialize(π, decimals=2)  → "3.14"  
serialize(π, decimals=6)  → "3.141593"
serialize(π, decimals=15) → "3.14159265358979"
serialize(π, decimals=17) → "3.1415926535897931" (full precision)
```

- [ ] Test precision 0-17 for 1000 random doubles
- [ ] Verify rounding, not truncation
- [ ] Verify no trailing zeros (unless needed)
- [ ] Verify parse(serialize(x, prec=17)) == x

#### 6.2 Shortest Representation
**Goal:** Verify we output shortest decimal that roundtrips.

```cpp
// Example: 0.1 (not exactly representable)
0.1 → "0.1" (not "0.10000000000000001")

// Example: 0.3
0.3 → "0.3" (not "0.29999999999999999")
```

- [ ] Compare against Ryu/Dragonbox (gold standard)
- [ ] For 1M random doubles, verify shortest form
- [ ] Verify parse(shortest(x)) == x

---

### 7. Performance Regression Tests [LOW PRIORITY]

**Goal:** Ensure correctness improvements don't harm performance.

#### 7.1 Benchmark Suite
- [ ] Measure parse throughput (numbers/sec)
  - Random JSON numbers (various lengths)
  - Powers of 10
  - Subnormals
- [ ] Measure serialize throughput
  - Random doubles
  - Various precision levels
- [ ] Compare to baselines: strtod, snprintf, Ryu, fast_float

#### 7.2 Real-World Data
- [ ] Benchmark on twitter.json (existing)
- [ ] Benchmark on canada.json (existing)
- [ ] Benchmark on numeric-heavy synthetic data
- [ ] Document: "slower than fast_float, but good enough"

**Acceptance:**
- Within 2-5x of fast_float on numeric benchmarks
- Competitive on mixed JSON workloads (Twitter)
- Acceptable for embedded use cases

---

### 8. Fuzzing and Chaos Testing [ONGOING]

#### 8.1 Extended Fuzz Campaigns
- [ ] Run existing fuzzer for 1 billion iterations
  - Currently at 100M × 10 error thresholds
- [ ] AFL/libFuzzer integration
  - Parse arbitrary byte sequences
  - Detect crashes/hangs/UB
- [ ] Longer decimal strings (up to buffer limit)
- [ ] Pathological exponents (e+999999999)

#### 8.2 Adversarial Inputs
- [ ] Generate inputs maximizing ULP error
- [ ] Test parser buffer boundaries
- [ ] Test with all-zero/all-nine strings
- [ ] Test with very long mantissas (50+ digits)

---

### 9. Constexpr Validation [CRITICAL]

**Goal:** Ensure all correctness holds in constexpr context.

- [ ] Port ULP tests to constexpr
- [ ] Port boundary tests to constexpr  
- [ ] Verify all edge cases work at compile time
- [ ] Add to `tests/constexpr/primitives/test_parse_float.cpp`
- [ ] Add to `tests/constexpr/serialization/test_serialize_float.cpp`

**Acceptance:**
- All tests pass in both runtime and constexpr
- No behavior differences between contexts

---

### 10. Documentation and Examples [LOW PRIORITY]

- [ ] Document accuracy guarantees (≤ 1 ULP)
- [ ] Document limitations vs fast_float
- [ ] Provide accuracy test results summary
- [ ] Create comparison table: accuracy × performance × dependencies
- [ ] Add examples of edge cases and their handling

---

## Test Suite Structure

```
tests/fp/
├── main.cpp                    # Existing fuzz tests (100M iterations)
├── ulp_tests.cpp              # NEW: ULP-based correctness
├── boundary_tests.cpp         # NEW: IEEE-754 edge cases
├── cross_library_tests.cpp    # NEW: Ryu/Dragonbox/double-conversion
├── stress_tests.cpp           # NEW: Rounding/exponent/subnormal stress
├── json_edge_cases.cpp        # NEW: RFC 8259 compliance
├── precision_tests.cpp        # NEW: Serialization precision
├── regression_bench.cpp       # NEW: Performance tracking
└── data/
    ├── gay_test_suite.txt     # David Gay's difficult cases
    ├── powers_of_2.txt        # Generated boundary cases
    └── subnormals.txt         # Subnormal test cases
```

---

## Implementation Priority

### Phase 1: Correctness Foundation [Week 1-2]
1. ✅ Implement ULP distance calculation
2. ✅ Import David Gay test suite (1000+ cases)
3. ✅ Powers of 2 boundary tests
4. ✅ Subnormal tests
5. ✅ Exponent extremes tests

### Phase 2: Cross-Validation [Week 2-3]
1. ✅ Integrate Ryu (serialization reference)
2. ✅ Integrate Dragonbox (alternative reference)
3. ✅ Compare against 1M random doubles
4. ✅ Document any discrepancies

### Phase 3: Stress Testing [Week 3-4]
1. ✅ Rounding boundary stress (10K midpoints)
2. ✅ Powers of 10 stress (1000+ combinations)
3. ✅ Extended fuzz (1B iterations)
4. ✅ Adversarial inputs

### Phase 4: Validation & Documentation [Week 4]
1. ✅ Constexpr test coverage
2. ✅ Performance regression suite
3. ✅ Documentation
4. ✅ Final decision: make in-house parser default

---

## Success Criteria

**Correctness:**
- ✅ ≤ 1 ULP error for 99.99% of inputs (vs reference implementations)
- ✅ All David Gay test suite passes
- ✅ All boundary tests pass
- ✅ 1B+ fuzz iterations without failures

**Completeness:**
- ✅ Constexpr works for all tested cases
- ✅ RFC 8259 fully compliant
- ✅ All IEEE-754 edge cases handled

**Performance:**
- ✅ Within 2-5x of fast_float
- ✅ Competitive on real-world workloads

**Confidence:**
- ✅ Tested against 3+ reference implementations
- ✅ Zero external dependencies validated
- ✅ Ready to be default parser

---

## References

1. **David Gay's dtoa.c:** http://www.netlib.org/fp/dtoa.c
2. **Ryu Algorithm:** https://github.com/ulfjack/ryu
3. **Dragonbox:** https://github.com/jk-jeon/dragonbox
4. **fast_float:** https://github.com/fastfloat/fast_float
5. **double-conversion:** https://github.com/google/double-conversion
6. **IEEE-754 Standard:** https://ieeexplore.ieee.org/document/8766229
7. **RFC 8259 (JSON):** https://tools.ietf.org/html/rfc8259
8. **Goldberg's "What Every Computer Scientist Should Know About Floating-Point"**
9. **Comparing Floating-Point Numbers (Bruce Dawson):** https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/


