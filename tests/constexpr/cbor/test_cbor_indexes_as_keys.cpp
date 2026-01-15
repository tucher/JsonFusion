#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/cbor.hpp>
#include <JsonFusion/annotated.hpp>
#include <JsonFusion/options.hpp>
#include <array>
#include <optional>

using namespace JsonFusion;
using namespace JsonFusion::options;

// ============================================================================
// indexes_as_keys is primarily designed for CBOR where integer keys are native
// It uses field indices (0, 1, 2...) as keys automatically, saving bandwidth
// ============================================================================

// Test structures at namespace scope
struct Point {
    int x;
    int y;

    constexpr bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

struct Data {
    int first;
    bool second;
    int third;

    constexpr bool operator==(const Data& other) const {
        return first == other.first
            && second == other.second
            && third == other.third;
    }
};

struct Inner {
    int a;
    int b;

    constexpr bool operator==(const Inner& other) const {
        return a == other.a && b == other.b;
    }
};

struct Outer {
    A<Inner, indexes_as_keys> nested;
    int regular;

    constexpr bool operator==(const Outer& other) const {
        return nested == other.nested && regular == other.regular;
    }
};

// ============================================================================
// Helper for CBOR roundtrip with indexes_as_keys
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

template<typename T>
constexpr std::size_t cbor_serialize_size(const T& obj) {
    std::array<char, 256> buffer{};
    auto it = buffer.begin();
    auto res = SerializeWithWriter(obj, CborWriter(it, buffer.end()));
    return res ? res.bytesWritten() : 0;
}

// ============================================================================
// Test: indexes_as_keys basic roundtrip
// ============================================================================

static_assert([]() constexpr {
    A<Point, indexes_as_keys> p{};
    p.value.x = 10;
    p.value.y = 20;
    return cbor_roundtrip(p);
}(), "indexes_as_keys should roundtrip through CBOR");

// ============================================================================
// Test: indexes_as_keys with multiple types
// ============================================================================

static_assert([]() constexpr {
    A<Data, indexes_as_keys> d{};
    d.value.first = 42;
    d.value.second = true;
    d.value.third = -100;
    return cbor_roundtrip(d);
}(), "indexes_as_keys should roundtrip mixed types through CBOR");

// ============================================================================
// Test: indexes_as_keys nested in regular struct
// ============================================================================

static_assert([]() constexpr {
    A<Outer, indexes_as_keys> o{};
    o.value.nested.value.a = 10;
    o.value.nested.value.b = 20;
    o.value.regular = 30;
    return cbor_roundtrip(o);
}(), "indexes_as_keys should work in nested CBOR structures");

// ============================================================================
// Test: indexes_as_keys is more compact than string keys
// ============================================================================

static_assert([]() constexpr {
    // With indexes_as_keys: keys are 0, 1, 2 (integers)
    A<Data, indexes_as_keys> compact{};
    compact.value.first = 42;
    compact.value.second = true;
    compact.value.third = -100;

    // Without: keys are "first", "second", "third" (strings)
    Data verbose{};
    verbose.first = 42;
    verbose.second = true;
    verbose.third = -100;

    std::size_t compact_size = cbor_serialize_size(compact);
    std::size_t verbose_size = cbor_serialize_size(verbose);

    // Integer keys (0, 1, 2) should be much smaller than string keys
    return compact_size > 0 && verbose_size > 0 && compact_size < verbose_size;
}(), "indexes_as_keys should produce more compact CBOR than string keys");

// ============================================================================
// Test: Verify significant size savings
// String keys "first" + "second" + "third" = 5+6+5 = 16 chars + headers
// Integer keys 0, 1, 2 = 3 bytes total
// ============================================================================

static_assert([]() constexpr {
    A<Data, indexes_as_keys> compact{};
    compact.value.first = 42;
    compact.value.second = true;
    compact.value.third = -100;

    Data verbose{};
    verbose.first = 42;
    verbose.second = true;
    verbose.third = -100;

    std::size_t compact_size = cbor_serialize_size(compact);
    std::size_t verbose_size = cbor_serialize_size(verbose);

    // String keys add at least 15 bytes more than integer keys 0,1,2
    return (verbose_size - compact_size) >= 15;
}(), "indexes_as_keys saves at least 15 bytes vs string keys");

// ============================================================================
// Test: indexes_as_keys with optional field
// ============================================================================

struct WithOptional {
    int required_field;
    std::optional<int> optional_field;

    constexpr bool operator==(const WithOptional& other) const {
        return required_field == other.required_field
            && optional_field == other.optional_field;
    }
};

static_assert([]() constexpr {
    A<WithOptional, indexes_as_keys> w{};
    w.value.required_field = 100;
    w.value.optional_field = 200;
    return cbor_roundtrip(w);
}(), "indexes_as_keys should work with optional fields present");

static_assert([]() constexpr {
    A<WithOptional, indexes_as_keys> w{};
    w.value.required_field = 100;
    w.value.optional_field = std::nullopt;
    return cbor_roundtrip(w);
}(), "indexes_as_keys should work with optional fields absent");

// ============================================================================
// Test: indexes_as_keys with array field
// ============================================================================

struct WithArray {
    std::array<int, 3> values;
    int count;

    constexpr bool operator==(const WithArray& other) const {
        return values == other.values && count == other.count;
    }
};

static_assert([]() constexpr {
    A<WithArray, indexes_as_keys> w{};
    w.value.values = {1, 2, 3};
    w.value.count = 3;
    return cbor_roundtrip(w);
}(), "indexes_as_keys should work with array fields");

