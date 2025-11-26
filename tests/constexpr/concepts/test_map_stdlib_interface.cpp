#include <JsonFusion/static_schema.hpp>
#include <JsonFusion/parser.hpp>
#include <array>

using namespace JsonFusion;
using namespace JsonFusion::static_schema;

// ============================================================================
// std::map-Compatible Interface Tests
// ============================================================================
//
// This file tests the automatic cursor adapters for std::map-like types.
// These are types that provide:
// - For parsing: try_emplace() and clear() methods
// - For serialization: begin()/end() iterators with std::pair<K,V>
//
// These allow direct use of std::map, std::unordered_map, and similar
// standard library containers WITHOUT any custom code.
// ============================================================================

// ============================================================================
// Test 1: try_emplace Interface (std::map-like, for Parsing)
// ============================================================================

/// std::map-like interface for parsing
/// The library provides automatic map_write_cursor for this pattern
template<typename K, typename V, std::size_t N>
struct StdMapLikeWrite {
    using key_type = K;
    using mapped_type = V;
    
    std::array<K, N> keys{};
    std::array<V, N> values{};
    std::size_t count = 0;
    
    // std::map-like interface: try_emplace
    constexpr auto try_emplace(K&& k, V&& v) {
        if (count < N) {
            keys[count] = k;
            values[count] = v;
            count++;
            return std::pair{keys.begin() + (count - 1), true};
        }
        return std::pair{keys.begin(), false};
    }
    
    // std::map-like interface: clear
    constexpr void clear() { count = 0; }
};

// Verify the interface is recognized by MapWritable concept
static_assert(MapWritable<StdMapLikeWrite<std::array<char, 16>, int, 10>>);
static_assert(JsonParsableMap<StdMapLikeWrite<std::array<char, 16>, int, 10>>);

// Verify cursor is automatically provided
static_assert(requires {
    typename map_write_cursor<StdMapLikeWrite<std::array<char, 16>, int, 10>>::key_type;
    typename map_write_cursor<StdMapLikeWrite<std::array<char, 16>, int, 10>>::mapped_type;
});

// Verify cursor has correct types
static_assert(std::is_same_v<
    typename map_write_cursor<StdMapLikeWrite<std::array<char, 32>, int, 10>>::key_type,
    std::array<char, 32>
>);

static_assert(std::is_same_v<
    typename map_write_cursor<StdMapLikeWrite<std::array<char, 32>, int, 10>>::mapped_type,
    int
>);

// ============================================================================
// Test 2: Iterator Interface (std::map-like, for Serialization)
// ============================================================================

/// std::map-like interface for serialization
/// The library provides automatic map_read_cursor for this pattern
template<typename K, typename V, std::size_t N>
struct StdMapLikeRead {
    using key_type = K;
    using mapped_type = V;
    using const_iterator = const std::pair<K, V>*;
    
    std::array<std::pair<K, V>, N> entries{};
    std::size_t count = 0;
    
    // std::map-like interface: iterators
    constexpr const_iterator begin() const { return entries.data(); }
    constexpr const_iterator end() const { return entries.data() + count; }
};

// Verify the interface is recognized by MapReadable concept
static_assert(MapReadable<StdMapLikeRead<std::array<char, 16>, int, 10>>);
static_assert(JsonSerializableMap<StdMapLikeRead<std::array<char, 16>, int, 10>>);

// Verify cursor is automatically provided
static_assert(requires {
    typename map_read_cursor<StdMapLikeRead<std::array<char, 16>, int, 10>>::key_type;
    typename map_read_cursor<StdMapLikeRead<std::array<char, 16>, int, 10>>::mapped_type;
});

// Verify cursor has correct types
static_assert(std::is_same_v<
    typename map_read_cursor<StdMapLikeRead<std::array<char, 32>, int, 10>>::key_type,
    std::array<char, 32>
>);

static_assert(std::is_same_v<
    typename map_read_cursor<StdMapLikeRead<std::array<char, 32>, int, 10>>::mapped_type,
    int
>);

// ============================================================================
// Test 3: Complex Value Types Work with std::map Interface
// ============================================================================

struct ComplexValue {
    int id;
    std::array<char, 32> name;
    std::array<int, 5> scores;
};

using ComplexMapWrite = StdMapLikeWrite<std::array<char, 32>, ComplexValue, 10>;
using ComplexMapRead = StdMapLikeRead<std::array<char, 32>, ComplexValue, 10>;

static_assert(JsonParsableValue<ComplexValue>);
static_assert(MapWritable<ComplexMapWrite>);
static_assert(JsonParsableMap<ComplexMapWrite>);
static_assert(MapReadable<ComplexMapRead>);
static_assert(JsonSerializableMap<ComplexMapRead>);

// ============================================================================
// Test 4: Nested Maps with std::map Interface
// ============================================================================

using InnerMap = StdMapLikeWrite<std::array<char, 16>, int, 10>;
using NestedMapWrite = StdMapLikeWrite<std::array<char, 32>, InnerMap, 5>;

static_assert(JsonParsableMap<InnerMap>);
static_assert(JsonParsableMap<NestedMapWrite>);

// ============================================================================
// Test 5: Optional Values with std::map Interface
// ============================================================================

using OptionalMapWrite = StdMapLikeWrite<std::array<char, 32>, std::optional<int>, 10>;
using OptionalMapRead = StdMapLikeRead<std::array<char, 32>, std::optional<int>, 10>;

static_assert(JsonParsableValue<std::optional<int>>);
static_assert(JsonParsableMap<OptionalMapWrite>);
static_assert(JsonSerializableMap<OptionalMapRead>);

// ============================================================================
// Test 6: Array Values with std::map Interface
// ============================================================================

using ArrayMapWrite = StdMapLikeWrite<std::array<char, 32>, std::array<int, 10>, 5>;
using ArrayMapRead = StdMapLikeRead<std::array<char, 32>, std::array<int, 10>, 5>;

static_assert(JsonParsableArray<std::array<int, 10>>);
static_assert(JsonParsableMap<ArrayMapWrite>);
static_assert(JsonSerializableMap<ArrayMapRead>);

// ============================================================================
// Test 7: Both Interfaces Can Coexist
// ============================================================================

// Type that supports BOTH parsing and serialization (std::map-like)
template<typename K, typename V, std::size_t N>
struct StdMapLikeFull {
    using key_type = K;
    using mapped_type = V;
    using const_iterator = const std::pair<K, V>*;
    
    std::array<std::pair<K, V>, N> entries{};
    std::size_t count = 0;
    
    // Parsing interface
    constexpr auto try_emplace(K&& k, V&& v) {
        if (count < N) {
            entries[count] = std::pair{k, v};
            count++;
            return std::pair{&entries[count - 1], true};
        }
        return std::pair{&entries[0], false};
    }
    constexpr void clear() { count = 0; }
    
    // Serialization interface
    constexpr const_iterator begin() const { return entries.data(); }
    constexpr const_iterator end() const { return entries.data() + count; }
};

using BidirectionalMap = StdMapLikeFull<std::array<char, 32>, int, 10>;

static_assert(MapWritable<BidirectionalMap>);
static_assert(MapReadable<BidirectionalMap>);
static_assert(JsonParsableMap<BidirectionalMap>);
static_assert(JsonSerializableMap<BidirectionalMap>);

// ============================================================================
// All std::map Interface Tests Pass!
// ============================================================================

constexpr bool all_stdmap_tests_pass = true;
static_assert(all_stdmap_tests_pass, "[[[ All std::map-compatible interface tests must pass ]]]");

int main() {
    // All tests verified at compile time
    return 0;
}

