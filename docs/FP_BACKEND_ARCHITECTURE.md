# Floating-Point Backend Architecture

## Overview

JsonFusion uses an in-house constexpr FP parser by default. This document describes the backend switch for rare edge cases.

```cpp
// include/JsonFusion/fp_to_str.hpp
#ifndef JSONFUSION_FP_BACKEND
#define JSONFUSION_FP_BACKEND 0  // in-house default
#endif
```

**API:**
```cpp
namespace JsonFusion::fp_to_str_detail {
    constexpr bool parse_number_to_double(const char* buf, double& out);
    constexpr char* format_double_to_chars(char* first, double value, std::size_t decimals);
}
```

---

## Backend 0: In-House (Default)

**Characteristics:**
- Fully `constexpr`
- Zero dependencies (no libc, no external libs)
- ~16KB on ARM Cortex-M7 (includes full parser)
- Correctly rounded parsing when the significant digits fit in 2^53 and the decimal exponent is within ±22 (typical hand-written values such as `0.3` or `123.456`); outside that range, e.g. 17-digit round-trip output, a few ULP off
- Correct rounding assumes IEEE double arithmetic without extended-precision evaluation (`FLT_EVAL_METHOD == 0`) and without `-ffast-math`
- 2-5x slower than fast_float on FP-heavy workloads
- Competitive on mixed JSON (Twitter, Canada benchmarks)

---

## Backend 1: fast_float + simdjson

```cpp
#define JSONFUSION_FP_BACKEND 1
```

**Characteristics:**
- Runtime-only (not constexpr)
- Requires fast_float + simdjson headers (bundled)
- Best-in-class FP accuracy
- 2-5x faster FP parsing than backend 0

**Note:** For performance, use yyjson backend instead (entire parser is faster). Backend 1 is for academic comparisons or FP-specific testing.

---

## Backend 2+: Custom

```cpp
#define JSONFUSION_FP_BACKEND 2
#include <JsonFusion/fp_to_str.hpp>

namespace JsonFusion::fp_to_str_detail {
    inline bool parse_number_to_double(const char* buf, double& out) {
        // Your implementation
    }
    
    inline char* format_double_to_chars(char* first, double value, std::size_t decimals) {
        // Your implementation
    }
}
```

**Use cases:** Fixed-point arithmetic, decimal types, platform-specific FP libraries.

---

## Comparison

| Feature | Backend 0 | Backend 1 | Backend 2+ |
|---------|-----------|-----------|-----------|
| Constexpr | ✅ | ❌ | Implementation-dependent |
| Dependencies | Zero | fast_float + simdjson | Implementation-dependent |
| Accuracy | Correctly rounded for typical values, a few ULP for long mantissas | Best-in-class | Implementation-dependent |
| Performance | 2-5x slower FP | Maximum FP | Implementation-dependent |
| Use Case | Default | Academic/testing | Fixed-point/decimal |

---

## Recommendations

- **Default:** Use backend 0 (works for 99% of cases)
- **Performance:** Use yyjson backend (not backend 1)
- **Exotic needs:** Implement backend 2+ (custom)
