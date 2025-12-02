#include <JsonFusion/static_schema.hpp>
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <array>
#include <optional>

using namespace JsonFusion;
using namespace JsonFusion::static_schema;

// ============================================================================
// Type Definitions - One of Each Category
// ============================================================================

// 1. Primitive: Bool
using TestBool = bool;

// 2. Primitive: Integer Number
using TestInt = int;

// 3. Primitive: Float Number
using TestFloat = double;

// 4. Primitive: String (char array)
using TestString = std::array<char, 32>;

// 5. Object (aggregate struct)
struct TestObject {
    int x;
    int y;
};

// 6. Array (std::array)
using TestArray = std::array<int, 10>;

// 7. Map (custom map type)
struct TestMap {
    using key_type = std::array<char, 32>;
    using mapped_type = int;
    
    constexpr auto try_emplace(key_type&& k, mapped_type&& v) {
        return std::pair{static_cast<const key_type*>(nullptr), true};
    }
    constexpr void clear() {}
};

// 8. Optional (nullable type)
using TestOptional = std::optional<int>;

// 9. Invalid type (pointer - should fail all concepts)
using TestPointer = int*;

// ============================================================================
// SECTION 1: JsonBool - Must ONLY match bool
// ============================================================================

namespace test_bool_concept {
    // Positive: bool IS JsonBool
    static_assert(JsonBool<bool>);
    
    // Negative: bool is NOT other concepts
    static_assert(!JsonNumber<bool>);
    static_assert(!JsonString<bool>);
    static_assert(!JsonObject<bool>);
    static_assert(!JsonParsableArray<bool>);
    static_assert(!JsonSerializableArray<bool>);
    static_assert(!JsonParsableMap<bool>);
    static_assert(!JsonSerializableMap<bool>);
    
    // Negative: Other types are NOT JsonBool
    static_assert(!JsonBool<int>);
    static_assert(!JsonBool<double>);
    static_assert(!JsonBool<TestString>);
    static_assert(!JsonBool<TestObject>);
    static_assert(!JsonBool<TestArray>);
    static_assert(!JsonBool<TestMap>);
    static_assert(!JsonBool<TestOptional>);
    static_assert(!JsonBool<TestPointer>);
}

// ============================================================================
// SECTION 2: JsonNumber - Must ONLY match numeric types
// ============================================================================

namespace test_number_concept {
    // Positive: numeric types ARE JsonNumber
    static_assert(JsonNumber<int>);
    static_assert(JsonNumber<unsigned int>);
    static_assert(JsonNumber<long>);
    static_assert(JsonNumber<short>);
    static_assert(JsonNumber<int8_t>);
    static_assert(JsonNumber<int16_t>);
    static_assert(JsonNumber<int32_t>);
    static_assert(JsonNumber<int64_t>);
    static_assert(JsonNumber<uint8_t>);
    static_assert(JsonNumber<uint16_t>);
    static_assert(JsonNumber<uint32_t>);
    static_assert(JsonNumber<uint64_t>);
    static_assert(JsonNumber<float>);
    static_assert(JsonNumber<double>);
    
    // Negative: numbers are NOT other concepts
    static_assert(!JsonBool<int>);
    static_assert(!JsonString<int>);
    static_assert(!JsonObject<int>);
    static_assert(!JsonParsableArray<int>);
    static_assert(!JsonSerializableArray<int>);
    static_assert(!JsonParsableMap<int>);
    static_assert(!JsonSerializableMap<int>);
    
    static_assert(!JsonBool<double>);
    static_assert(!JsonString<double>);
    static_assert(!JsonObject<double>);
    static_assert(!JsonParsableArray<double>);
    static_assert(!JsonSerializableArray<double>);
    static_assert(!JsonParsableMap<double>);
    static_assert(!JsonSerializableMap<double>);
    
    // Negative: Other types are NOT JsonNumber
    static_assert(!JsonNumber<bool>);
    static_assert(!JsonNumber<TestString>);
    static_assert(!JsonNumber<TestObject>);
    static_assert(!JsonNumber<TestArray>);
    static_assert(!JsonNumber<TestMap>);
    static_assert(!JsonNumber<TestOptional>);
    static_assert(!JsonNumber<TestPointer>);
}

// ============================================================================
// SECTION 3: JsonString - Must ONLY match string types
// ============================================================================

namespace test_string_concept {
    // Positive: string types ARE JsonString
    static_assert(JsonString<std::array<char, 32>>);
    static_assert(JsonString<std::array<char, 64>>);
    static_assert(JsonString<std::array<char, 1>>);
    
    // Negative: strings are NOT other concepts
    static_assert(!JsonBool<TestString>);
    static_assert(!JsonNumber<TestString>);
    static_assert(!JsonObject<TestString>);
    static_assert(!JsonParsableArray<TestString>);
    static_assert(!JsonSerializableArray<TestString>);
    static_assert(!JsonParsableMap<TestString>);
    static_assert(!JsonSerializableMap<TestString>);
    
    // Negative: Other types are NOT JsonString
    static_assert(!JsonString<bool>);
    static_assert(!JsonString<int>);
    static_assert(!JsonString<double>);
    static_assert(!JsonString<TestObject>);
    static_assert(!JsonString<std::array<int, 10>>);  // array of int, not char
    static_assert(!JsonString<TestMap>);
    static_assert(!JsonString<TestOptional>);
    static_assert(!JsonString<TestPointer>);
}

// ============================================================================
// SECTION 4: JsonObject - Must ONLY match aggregate structs (not maps/arrays)
// ============================================================================

namespace test_object_concept {
    // Positive: aggregate structs ARE JsonObject
    static_assert(JsonObject<TestObject>);
    
    struct AnotherObject {
        int a;
        bool b;
        std::array<char, 16> c;
    };
    static_assert(JsonObject<AnotherObject>);
    
    struct NestedObject {
        int x;
        TestObject inner;
    };
    static_assert(JsonObject<NestedObject>);
    
    // Negative: objects are NOT other concepts
    static_assert(!JsonBool<TestObject>);
    static_assert(!JsonNumber<TestObject>);
    static_assert(!JsonString<TestObject>);
    static_assert(!JsonParsableArray<TestObject>);
    static_assert(!JsonSerializableArray<TestObject>);
    static_assert(!JsonParsableMap<TestObject>);
    static_assert(!JsonSerializableMap<TestObject>);
    
    // CRITICAL: Objects are NOT maps (even if they look similar)
    static_assert(!JsonObject<TestMap>);
    
    // Negative: Other types are NOT JsonObject
    static_assert(!JsonObject<bool>);
    static_assert(!JsonObject<int>);
    static_assert(!JsonObject<double>);
    static_assert(!JsonObject<TestString>);
    static_assert(!JsonObject<TestArray>);
    static_assert(!JsonObject<TestMap>);
    static_assert(!JsonObject<TestOptional>);
    static_assert(!JsonObject<TestPointer>);
}

// ============================================================================
// SECTION 5: JsonParsableArray - Must ONLY match array types
// ============================================================================

namespace test_array_concept {
    // Positive: arrays ARE JsonParsableArray
    static_assert(JsonParsableArray<std::array<int, 10>>);
    static_assert(JsonParsableArray<std::array<bool, 5>>);
    static_assert(JsonParsableArray<std::array<TestObject, 3>>);
    static_assert(JsonParsableArray<std::array<std::array<int, 5>, 3>>);  // nested arrays
    
    // Negative: arrays are NOT other concepts
    static_assert(!JsonBool<TestArray>);
    static_assert(!JsonNumber<TestArray>);
    static_assert(!JsonString<TestArray>);  // array<int>, not array<char>
    static_assert(!JsonObject<TestArray>);
    static_assert(!JsonParsableMap<TestArray>);
    static_assert(!JsonSerializableMap<TestArray>);
    
    // CRITICAL: Arrays are NOT objects
    static_assert(!JsonObject<TestArray>);
    
    // CRITICAL: Arrays are NOT maps
    static_assert(!JsonParsableMap<TestArray>);
    
    // Negative: Other types are NOT JsonParsableArray
    static_assert(!JsonParsableArray<bool>);
    static_assert(!JsonParsableArray<int>);
    static_assert(!JsonParsableArray<double>);
    static_assert(!JsonParsableArray<TestString>);  // char array is string, not array
    static_assert(!JsonParsableArray<TestObject>);
    static_assert(!JsonParsableArray<TestMap>);
    static_assert(!JsonParsableArray<TestOptional>);
    static_assert(!JsonParsableArray<TestPointer>);
}

// ============================================================================
// SECTION 6: JsonParsableMap - Must ONLY match map types
// ============================================================================

namespace test_map_concept {
    // Positive: maps ARE JsonParsableMap
    static_assert(JsonParsableMap<TestMap>);
    
    struct AnotherMap {
        using key_type = std::array<char, 16>;
        using mapped_type = bool;
        
        constexpr auto try_emplace(key_type&& k, mapped_type&& v) {
            return std::pair{static_cast<const key_type*>(nullptr), true};
        }
        constexpr void clear() {}
    };
    static_assert(JsonParsableMap<AnotherMap>);
    
    // Map with struct value
    struct MapWithStructValue {
        using key_type = std::array<char, 32>;
        using mapped_type = TestObject;
        
        constexpr auto try_emplace(key_type&& k, mapped_type&& v) {
            return std::pair{static_cast<const key_type*>(nullptr), true};
        }
        constexpr void clear() {}
    };
    static_assert(JsonParsableMap<MapWithStructValue>);
    
    // Negative: maps are NOT other concepts
    static_assert(!JsonBool<TestMap>);
    static_assert(!JsonNumber<TestMap>);
    static_assert(!JsonString<TestMap>);
    static_assert(!JsonObject<TestMap>);  // CRITICAL: Maps are NOT objects!
    static_assert(!JsonParsableArray<TestMap>);
    static_assert(!JsonSerializableArray<TestMap>);
    
    // CRITICAL: Maps are NOT objects (this was the bug!)
    static_assert(!JsonObject<TestMap>);
    static_assert(!JsonObject<AnotherMap>);
    static_assert(!JsonObject<MapWithStructValue>);
    
    // CRITICAL: Maps are NOT arrays
    static_assert(!JsonParsableArray<TestMap>);
    
    // Negative: Other types are NOT JsonParsableMap
    static_assert(!JsonParsableMap<bool>);
    static_assert(!JsonParsableMap<int>);
    static_assert(!JsonParsableMap<double>);
    static_assert(!JsonParsableMap<TestString>);
    static_assert(!JsonParsableMap<TestObject>);  // Objects are not maps
    static_assert(!JsonParsableMap<TestArray>);
    static_assert(!JsonParsableMap<TestOptional>);
    static_assert(!JsonParsableMap<TestPointer>);
    
    // Map with invalid key type (not string)
    struct InvalidKeyMap {
        using key_type = int;  // Invalid!
        using mapped_type = int;
        
        constexpr auto try_emplace(key_type&& k, mapped_type&& v) {
            return std::pair{static_cast<const key_type*>(nullptr), true};
        }
        constexpr void clear() {}
    };
    
    // Structurally valid but not a JSON map (keys must be strings)
    static_assert(!JsonParsableMap<InvalidKeyMap>);
    static_assert(!JsonParsableValue<InvalidKeyMap>);  // Not parsable (invalid key type)
    static_assert(!JsonObject<InvalidKeyMap>);  // Also not an object
    static_assert(!JsonParsableArray<InvalidKeyMap>);  // Also not an array
}

// ============================================================================
// SECTION 7: Test ConsumingStreamerLike and ProducingStreamerLike
// ============================================================================

namespace test_streamer_concepts {
    // ConsumingStreamerLike - for parsing (array-like)
    template<typename T>
    struct SimpleConsumer {
        using value_type = T;
        
        std::array<T, 10> items{};
        std::size_t count = 0;
        
        constexpr bool consume(const T& item) {
            if (count >= 10) return false;
            items[count++] = item;
            return true;
        }
        
        constexpr bool finalize(bool success) {
            return success;
        }
        
        constexpr void reset() {
            count = 0;
        }
    };
    
    using TestConsumer = SimpleConsumer<int>;
    
    // CRITICAL: ConsumingStreamerLike should be detected as ARRAY
    static_assert(ConsumingStreamerLike<TestConsumer>);
    static_assert(JsonParsableArray<TestConsumer>);
    static_assert(JsonParsableValue<TestConsumer>);  // Highest level concept
    
    // ConsumingStreamerLike is NOT other concepts
    static_assert(!JsonObject<TestConsumer>);
    static_assert(!JsonParsableMap<TestConsumer>);
    static_assert(!JsonBool<TestConsumer>);
    static_assert(!JsonNumber<TestConsumer>);
    static_assert(!JsonString<TestConsumer>);
    
    // ProducingStreamerLike - for serialization (array-like)
    template<typename T>
    struct SimpleProducer {
        using value_type = T;
        
        const std::array<T, 5>* items;
        std::size_t count;
        mutable std::size_t index = 0;
        
        constexpr SimpleProducer(const std::array<T, 5>& arr, std::size_t cnt)
            : items(&arr), count(cnt) {}
        
        constexpr stream_read_result read(T& item) const {
            if (index >= count) {
                return stream_read_result::end;
            }
            item = (*items)[index++];
            return stream_read_result::value;
        }
        
        constexpr void reset() const {
            index = 0;
        }
    };
    
    using TestProducer = SimpleProducer<int>;
    
    // CRITICAL: ProducingStreamerLike should be detected as ARRAY
    static_assert(ProducingStreamerLike<TestProducer>);
    static_assert(JsonSerializableArray<TestProducer>);
    static_assert(JsonSerializableValue<TestProducer>);  // Highest level concept
    
    // ProducingStreamerLike is NOT other concepts
    static_assert(!JsonObject<TestProducer>);
    static_assert(!JsonSerializableMap<TestProducer>);
    static_assert(!JsonBool<TestProducer>);
    static_assert(!JsonNumber<TestProducer>);
    static_assert(!JsonString<TestProducer>);
    
    // Test with complex value_type
    struct Point { int x; int y; };
    
    using PointConsumer = SimpleConsumer<Point>;
    
    static_assert(ConsumingStreamerLike<PointConsumer>);
    static_assert(JsonParsableArray<PointConsumer>);
    static_assert(!JsonObject<PointConsumer>);
    
    // IMPORTANT: StreamerLike types use ARRAY interface, not MAP
    // Even though they have different extension point than cursor-based types
    static_assert(!JsonParsableMap<TestConsumer>);
    static_assert(!JsonSerializableMap<TestProducer>);
}

// ============================================================================
// SECTION 8: Test ConsumingMapStreamerLike and ProducingMapStreamerLike
// ============================================================================

namespace test_map_streamer_concepts {
    // Map entry type - must have .key and .value members
    template<typename K, typename V>
    struct MapEntry {
        K key;
        V value;
        
        constexpr bool operator==(const MapEntry& other) const {
            return key == other.key && value == other.value;
        }
    };
    
    // ConsumingMapStreamerLike - for parsing maps
    template<typename K, typename V, std::size_t MaxEntries>
    struct SimpleMapConsumer {
        using value_type = MapEntry<K, V>;
        
        std::array<MapEntry<K, V>, MaxEntries> entries{};
        std::size_t count = 0;
        
        constexpr bool consume(const MapEntry<K, V>& entry) {
            if (count >= MaxEntries) return false;
            entries[count++] = entry;
            return true;
        }
        
        constexpr bool finalize(bool success) {
            return success;
        }
        
        constexpr void reset() {
            count = 0;
        }
    };
    
    using TestMapConsumer = SimpleMapConsumer<std::array<char, 32>, int, 10>;
    
    // CRITICAL: ConsumingMapStreamerLike should be detected as MAP
    static_assert(ConsumingMapStreamerLike<TestMapConsumer>);
    static_assert(JsonParsableMap<TestMapConsumer>);
    static_assert(JsonParsableValue<TestMapConsumer>);  // Highest level concept
    
    // ConsumingMapStreamerLike is NOT other concepts
    static_assert(!JsonObject<TestMapConsumer>);
    static_assert(!JsonParsableArray<TestMapConsumer>);
    static_assert(!JsonBool<TestMapConsumer>);
    static_assert(!JsonNumber<TestMapConsumer>);
    static_assert(!JsonString<TestMapConsumer>);
    
    // ProducingMapStreamerLike - for serializing maps
    template<typename K, typename V, std::size_t N>
    struct SimpleMapProducer {
        using value_type = MapEntry<K, V>;
        
        const std::array<MapEntry<K, V>, N>* entries;
        std::size_t count;
        mutable std::size_t index = 0;
        
        constexpr SimpleMapProducer(const std::array<MapEntry<K, V>, N>& arr, std::size_t cnt)
            : entries(&arr), count(cnt) {}
        
        constexpr stream_read_result read(MapEntry<K, V>& entry) const {
            if (index >= count) {
                return stream_read_result::end;
            }
            entry = (*entries)[index++];
            return stream_read_result::value;
        }
        
        constexpr void reset() const {
            index = 0;
        }
    };
    
    using TestMapProducer = SimpleMapProducer<std::array<char, 32>, int, 5>;
    
    // CRITICAL: ProducingMapStreamerLike should be detected as MAP
    static_assert(ProducingMapStreamerLike<TestMapProducer>);
    static_assert(JsonSerializableMap<TestMapProducer>);
    static_assert(JsonSerializableValue<TestMapProducer>);  // Highest level concept
    
    // ProducingMapStreamerLike is NOT other concepts
    static_assert(!JsonObject<TestMapProducer>);
    static_assert(!JsonSerializableArray<TestMapProducer>);
    static_assert(!JsonBool<TestMapProducer>);
    static_assert(!JsonNumber<TestMapProducer>);
    static_assert(!JsonString<TestMapProducer>);
    
    // Test with struct value type
    struct Point { int x; int y; };
    
    using PointMapConsumer = SimpleMapConsumer<std::array<char, 16>, Point, 5>;
    
    static_assert(ConsumingMapStreamerLike<PointMapConsumer>);
    static_assert(JsonParsableMap<PointMapConsumer>);
    static_assert(!JsonObject<PointMapConsumer>);
    static_assert(!JsonParsableArray<PointMapConsumer>);
    
    // CRITICAL: Map streamers use MAP interface, not ARRAY
    static_assert(!JsonParsableArray<TestMapConsumer>);
    static_assert(!JsonSerializableArray<TestMapProducer>);
    
    // Test that invalid key types are rejected
    struct InvalidMapConsumer {
        struct BadEntry {
            int key;  // NOT a string!
            int value;
        };
        using value_type = BadEntry;
        
        constexpr bool consume(const BadEntry&) { return true; }
        constexpr bool finalize(bool) { return true; }
        constexpr void reset() {}
    };
    
    // Should NOT satisfy ConsumingMapStreamerLike (key is not string)
    static_assert(!ConsumingMapStreamerLike<InvalidMapConsumer>);
    static_assert(!JsonParsableMap<InvalidMapConsumer>);
    
    // Compare: Array streamers vs Map streamers
    // - Array streamers: value_type is the element
    // - Map streamers: value_type is an entry with .key and .value
    using ArrayConsumer = test_streamer_concepts::SimpleConsumer<int>;
    
    static_assert(ConsumingStreamerLike<ArrayConsumer>);
    static_assert(!ConsumingMapStreamerLike<ArrayConsumer>);  // No .key/.value
    
    static_assert(ConsumingMapStreamerLike<TestMapConsumer>);
    static_assert(!ConsumingStreamerLike<TestMapConsumer>);  // value_type not directly parsable
}

// ============================================================================
// SECTION 9: Test Nullable/Optional Types
// ============================================================================

namespace test_optional_concept {
    // Optionals should match their inner type's concept
    // But are handled specially for null values
    
    static_assert(JsonNullableParsableValue<std::optional<int>>);
    static_assert(!JsonNonNullableParsableValue<std::optional<int>>);
    
    static_assert(JsonNullableParsableValue<std::optional<bool>>);
    static_assert(JsonNullableParsableValue<std::optional<TestObject>>);
    static_assert(JsonNullableParsableValue<std::optional<TestArray>>);
    
    // Non-optionals are non-nullable
    static_assert(JsonNonNullableParsableValue<int>);
    static_assert(JsonNonNullableParsableValue<bool>);
    static_assert(JsonNonNullableParsableValue<TestString>);
    static_assert(JsonNonNullableParsableValue<TestObject>);
    static_assert(JsonNonNullableParsableValue<TestArray>);
    static_assert(JsonNonNullableParsableValue<TestMap>);
    
    static_assert(!JsonNullableParsableValue<int>);
    static_assert(!JsonNullableParsableValue<bool>);
    static_assert(!JsonNullableParsableValue<TestString>);
    static_assert(!JsonNullableParsableValue<TestObject>);
}

// ============================================================================
// SECTION 10: Comprehensive Type Classification Matrix
// ============================================================================

namespace test_classification_matrix {
    // Helper template to verify exactly one concept matches
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
    
    // Each valid type should match EXACTLY ONE primary concept
    static_assert(count_matching_concepts<bool>() == 1, "bool should match exactly 1 concept");
    static_assert(count_matching_concepts<int>() == 1, "int should match exactly 1 concept");
    static_assert(count_matching_concepts<double>() == 1, "double should match exactly 1 concept");
    static_assert(count_matching_concepts<TestString>() == 1, "string should match exactly 1 concept");
    static_assert(count_matching_concepts<TestObject>() == 1, "object should match exactly 1 concept");
    static_assert(count_matching_concepts<TestArray>() == 1, "array should match exactly 1 concept");
    static_assert(count_matching_concepts<TestMap>() == 1, "map should match exactly 1 concept");
    
    // Invalid types should match ZERO concepts
    static_assert(count_matching_concepts<TestPointer>() == 0, "pointer should match no concepts");
    // Note: void is excluded by is_directly_forbidden check and causes compilation errors
    // static_assert(count_matching_concepts<void>() == 0, "void should match no concepts");
}

// ============================================================================
// SECTION 11: Edge Cases and Corner Cases
// ============================================================================

namespace test_edge_cases {
    // Struct with only one field - still an object
    struct SingleFieldStruct {
        int value;
    };
    static_assert(JsonObject<SingleFieldStruct>);
    static_assert(!JsonParsableMap<SingleFieldStruct>);
    static_assert(!JsonParsableArray<SingleFieldStruct>);
    
    // Empty struct - still an object
    struct EmptyStruct {};
    static_assert(JsonObject<EmptyStruct>);
    static_assert(!JsonParsableMap<EmptyStruct>);
    static_assert(!JsonParsableArray<EmptyStruct>);
    
    // Struct with key_type and mapped_type but missing map interface
    // Should still be an OBJECT, not a MAP
    struct FakeMap {
        using key_type = int;
        using mapped_type = int;
        int x;
        int y;
    };
    // This doesn't have try_emplace/clear, so it's NOT a map
    static_assert(!JsonParsableMap<FakeMap>);
    static_assert(JsonObject<FakeMap>);  // It's just a regular struct
    static_assert(JsonParsableValue<FakeMap>);  // Parsable as object (highest level)
    
    // Array of size 1
    using SingletonArray = std::array<int, 1>;
    static_assert(JsonParsableArray<SingletonArray>);
    static_assert(!JsonObject<SingletonArray>);
    static_assert(!JsonParsableMap<SingletonArray>);
    
    // Array of size 0
    using EmptyArray = std::array<int, 0>;
    static_assert(JsonParsableArray<EmptyArray>);
    static_assert(!JsonObject<EmptyArray>);
    static_assert(!JsonParsableMap<EmptyArray>);
    
    // Deeply nested types
    struct DeeplyNested {
        std::array<std::array<std::optional<TestObject>, 3>, 5> data;
    };
    static_assert(JsonObject<DeeplyNested>);
    static_assert(!JsonParsableMap<DeeplyNested>);
    static_assert(!JsonParsableArray<DeeplyNested>);
}

// ============================================================================
// SECTION 12: Test Annotated Types
// ============================================================================

namespace test_annotated_types {
    using namespace JsonFusion::options;
    
    // Annotated types should preserve their underlying concept
    using AnnotatedInt = Annotated<int, key<"mykey">>;
    static_assert(JsonNumber<AnnotatedInt>);
    static_assert(!JsonBool<AnnotatedInt>);
    static_assert(!JsonObject<AnnotatedInt>);
    
    using AnnotatedString = Annotated<std::array<char, 32>, key<"name">>;
    static_assert(JsonString<AnnotatedString>);
    static_assert(!JsonNumber<AnnotatedString>);
    static_assert(!JsonObject<AnnotatedString>);
    
    using AnnotatedArray = Annotated<std::array<int, 10>, key<"items">>;
    static_assert(JsonParsableArray<AnnotatedArray>);
    static_assert(!JsonObject<AnnotatedArray>);
    static_assert(!JsonParsableMap<AnnotatedArray>);
}

// ============================================================================
// All Structural Detection Tests Pass!
// ============================================================================

constexpr bool all_structural_tests_pass = true;
static_assert(all_structural_tests_pass, "[[[ All structural detection tests must pass at compile time ]]]");

int main() {
    // All tests run at compile time via static_assert
    return 0;
}

