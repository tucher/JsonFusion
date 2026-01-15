#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/cbor.hpp>
#include <JsonFusion/annotated.hpp>
#include <JsonFusion/options.hpp>
#include <array>

using namespace JsonFusion;
using namespace JsonFusion::options;

// ============================================================================
// int_key is primarily designed for CBOR where integer keys are native
// CBOR maps can use integers as keys directly, making int_key efficient
// ============================================================================

// Test structures at namespace scope
struct Point {
    Annotated<int, int_key<0>> x;
    Annotated<int, int_key<1>> y;

    constexpr bool operator==(const Point& other) const {
        return x.value == other.x.value && y.value == other.y.value;
    }
};

struct Data {
    Annotated<int, int_key<0>> first;
    Annotated<bool, int_key<1>> second;
    Annotated<int, int_key<2>> third;

    constexpr bool operator==(const Data& other) const {
        return first.value == other.first.value
            && second.value == other.second.value
            && third.value == other.third.value;
    }
};

struct Sparse {
    Annotated<int, int_key<100>> field100;
    Annotated<int, int_key<255>> field255;

    constexpr bool operator==(const Sparse& other) const {
        return field100.value == other.field100.value
            && field255.value == other.field255.value;
    }
};

struct Inner {
    Annotated<int, int_key<0>> a;
    Annotated<int, int_key<1>> b;

    constexpr bool operator==(const Inner& other) const {
        return a.value == other.a.value && b.value == other.b.value;
    }
};

struct Outer {
    Inner nested;
    int regular;

    constexpr bool operator==(const Outer& other) const {
        return nested == other.nested && regular == other.regular;
    }
};

// Compact version with int_key
struct Compact {
    Annotated<int, int_key<0>> value;

    constexpr bool operator==(const Compact& other) const {
        return value.value == other.value.value;
    }
};

// Verbose version with string key
struct Verbose {
    int value;

    constexpr bool operator==(const Verbose& other) const {
        return value == other.value;
    }
};

// ============================================================================
// Helper for CBOR roundtrip
// ============================================================================

template<typename T>
constexpr bool cbor_roundtrip(const T& original) {
    std::array<char, 256> buffer{};
    auto it = buffer.begin();
    auto res = SerializeWithWriter(original, CborWriter(it, buffer.end()));
    if (!res) return false;

    T parsed{};
    if (!ParseWithReader(parsed, CborReader(buffer.begin(), buffer.begin() + res.bytesWritten())))
        return false;

    return original == parsed;
}

// ============================================================================
// Test: int_key basic roundtrip
// ============================================================================

static_assert([]() constexpr {
    Point p{};
    p.x.value = 10;
    p.y.value = 20;
    return cbor_roundtrip(p);
}(), "int_key should work with CBOR serialization");

// ============================================================================
// Test: int_key with multiple types
// ============================================================================

static_assert([]() constexpr {
    Data d{};
    d.first.value = 42;
    d.second.value = true;
    d.third.value = -100;
    return cbor_roundtrip(d);
}(), "int_key should roundtrip through CBOR with mixed types");

// ============================================================================
// Test: int_key with larger indices
// ============================================================================

static_assert([]() constexpr {
    Sparse s{};
    s.field100.value = 1;
    s.field255.value = 2;
    return cbor_roundtrip(s);
}(), "int_key should work with larger integer keys in CBOR");

// ============================================================================
// Test: int_key nested in CBOR
// ============================================================================

static_assert([]() constexpr {
    Outer o{};
    o.nested.a.value = 10;
    o.nested.b.value = 20;
    o.regular = 30;
    return cbor_roundtrip(o);
}(), "int_key should work in nested CBOR structures");

// ============================================================================
// Test: CBOR with int_key is more compact than string keys
// ============================================================================

static_assert([]() constexpr {
    Compact c{};
    c.value.value = 42;

    Verbose v{};
    v.value = 42;

    std::array<char, 256> compact_buffer{};
    auto compact_it = compact_buffer.begin();
    auto compact_res = SerializeWithWriter(c, CborWriter(compact_it, compact_buffer.end()));
    if (!compact_res) return false;
    std::size_t compact_size = compact_res.bytesWritten();

    std::array<char, 256> verbose_buffer{};
    auto verbose_it = verbose_buffer.begin();
    auto verbose_res = SerializeWithWriter(v, CborWriter(verbose_it, verbose_buffer.end()));
    if (!verbose_res) return false;
    std::size_t verbose_size = verbose_res.bytesWritten();

    // int_key version should be more compact (integer key vs "value" string)
    return compact_size < verbose_size;
}(), "int_key should produce more compact CBOR than string keys");

