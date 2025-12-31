#include <JsonFusion/static_schema.hpp>
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <array>
#include <optional>
#include <map>
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
struct TestCustomMap {
    using key_type = std::array<char, 32>;
    using mapped_type = int;
    
    constexpr auto try_emplace(key_type&& k, mapped_type&& v) {
        return std::pair{static_cast<const key_type*>(nullptr), true};
    }
    constexpr void clear() {}
};
using TestMap = std::map<std::array<char, 32>, int>;
using TestUnorderedMap = std::unordered_map<std::string, int>;

using TestCArray = int[10];
static_assert(ParsableArrayLike<TestCArray>);   
static_assert(SerializableArrayLike<TestCArray>);   

// 8. Optional (nullable type)
using TestOptional = std::optional<int>;
using TestUniquePtr = std::unique_ptr<int>;
// 9. Invalid type (pointer - should fail all concepts)
using TestPointer = int*;

// ============================================================================
// SECTION 1: BoolLike - Must ONLY match bool
// ============================================================================

namespace test_bool_concept {
    // Positive: bool IS BoolLike
    static_assert(BoolLike<bool>);
    
    // Negative: bool is NOT other concepts
    static_assert(!NumberLike<bool>);
    static_assert(!StringLike<bool>);
    static_assert(!ObjectLike<bool>);
    static_assert(!ParsableArrayLike<bool>);
    static_assert(!SerializableArrayLike<bool>);
    static_assert(!ParsableMapLike<bool>);
    static_assert(!JsonSerializableMap<bool>);
    
    // Negative: Other types are NOT BoolLike
    static_assert(!BoolLike<int>);
    static_assert(!BoolLike<double>);
    static_assert(!BoolLike<TestString>);
    static_assert(!BoolLike<TestObject>);
    static_assert(!BoolLike<TestArray>);
    static_assert(!BoolLike<TestCArray>);
    static_assert(!BoolLike<TestCustomMap>);
    static_assert(!BoolLike<TestUnorderedMap>);
    static_assert(!BoolLike<TestMap>);
    static_assert(!BoolLike<TestOptional>);
    static_assert(!BoolLike<TestUniquePtr>);
    static_assert(!BoolLike<TestPointer>);
}

// ============================================================================
// SECTION 2: NumberLike - Must ONLY match numeric types
// ============================================================================

namespace test_number_concept {
    // Positive: numeric types ARE NumberLike
    static_assert(NumberLike<int>);
    static_assert(NumberLike<unsigned int>);
    static_assert(NumberLike<long>);
    static_assert(NumberLike<short>);
    static_assert(NumberLike<int8_t>);
    static_assert(NumberLike<int16_t>);
    static_assert(NumberLike<int32_t>);
    static_assert(NumberLike<int64_t>);
    static_assert(NumberLike<uint8_t>);
    static_assert(NumberLike<uint16_t>);
    static_assert(NumberLike<uint32_t>);
    static_assert(NumberLike<uint64_t>);
    static_assert(NumberLike<float>);
    static_assert(NumberLike<double>);
    
    // Negative: numbers are NOT other concepts
    static_assert(!BoolLike<int>);
    static_assert(!StringLike<int>);
    static_assert(!ObjectLike<int>);
    static_assert(!ParsableArrayLike<int>);
    static_assert(!SerializableArrayLike<int>);
    static_assert(!ParsableMapLike<int>);
    static_assert(!JsonSerializableMap<int>);
    
    static_assert(!BoolLike<double>);
    static_assert(!StringLike<double>);
    static_assert(!ObjectLike<double>);
    static_assert(!ParsableArrayLike<double>);
    static_assert(!SerializableArrayLike<double>);
    static_assert(!ParsableMapLike<double>);
    static_assert(!JsonSerializableMap<double>);
    
    // Negative: Other types are NOT NumberLike
    static_assert(!NumberLike<bool>);
    static_assert(!NumberLike<TestString>);
    static_assert(!NumberLike<TestObject>);
    static_assert(!NumberLike<TestArray>);
    static_assert(!NumberLike<TestCArray>);
    static_assert(!NumberLike<TestCustomMap>);
    static_assert(!NumberLike<TestUnorderedMap>);
    static_assert(!NumberLike<TestMap>);
    static_assert(!NumberLike<TestOptional>);
    static_assert(!NumberLike<TestUniquePtr>);
    static_assert(!NumberLike<TestPointer>);
}

// ============================================================================
// SECTION 3: StringLike - Must ONLY match string types
// ============================================================================

namespace test_string_concept {
    // Positive: string types ARE StringLike
    static_assert(StringLike<std::array<char, 32>>);
    static_assert(StringLike<std::array<char, 64>>);
    static_assert(StringLike<std::array<char, 1>>);
    
    // Negative: strings are NOT other concepts
    static_assert(!BoolLike<TestString>);
    static_assert(!NumberLike<TestString>);
    static_assert(!ObjectLike<TestString>);
    static_assert(!ParsableArrayLike<TestString>);
    static_assert(!SerializableArrayLike<TestString>);
    static_assert(!ParsableMapLike<TestString>);
    static_assert(!JsonSerializableMap<TestString>);
    
    // Negative: Other types are NOT StringLike
    static_assert(!StringLike<bool>);
    static_assert(!StringLike<int>);
    static_assert(!StringLike<double>);
    static_assert(!StringLike<TestObject>);
    static_assert(!StringLike<std::array<int, 10>>);  // array of int, not char
    static_assert(!StringLike<TestCArray>);  // C array of int, not char
    static_assert(!StringLike<TestCustomMap>);
    static_assert(!StringLike<TestUnorderedMap>);
    static_assert(!StringLike<TestMap>);
    static_assert(!StringLike<TestOptional>);
    static_assert(!StringLike<TestUniquePtr>);
    static_assert(!StringLike<TestPointer>);
}

// ============================================================================
// SECTION 4: ObjectLike - Must ONLY match aggregate structs (not maps/arrays)
// ============================================================================

namespace test_object_concept {
    // Positive: aggregate structs ARE ObjectLike
    static_assert(ObjectLike<TestObject>);
    
    struct AnotherObject {
        int a;
        bool b;
        std::array<char, 16> c;
    };
    static_assert(ObjectLike<AnotherObject>);
    
    struct NestedObject {
        int x;
        TestObject inner;
    };
    static_assert(ObjectLike<NestedObject>);
    
    // Negative: objects are NOT other concepts
    static_assert(!BoolLike<TestObject>);
    static_assert(!NumberLike<TestObject>);
    static_assert(!StringLike<TestObject>);
    static_assert(!ParsableArrayLike<TestObject>);
    static_assert(!SerializableArrayLike<TestObject>);
    static_assert(!ParsableMapLike<TestObject>);
    static_assert(!JsonSerializableMap<TestObject>);
    
    // CRITICAL: Objects are NOT maps (even if they look similar)
    static_assert(!ObjectLike<TestCustomMap>);
    static_assert(!ObjectLike<TestUnorderedMap>);
    static_assert(!ObjectLike<TestMap>);
    
    // Negative: Other types are NOT ObjectLike
    static_assert(!ObjectLike<bool>);
    static_assert(!ObjectLike<int>);
    static_assert(!ObjectLike<double>);
    static_assert(!ObjectLike<TestString>);
    static_assert(!ObjectLike<TestArray>);
    static_assert(!ObjectLike<TestCArray>);
    static_assert(!ObjectLike<TestCustomMap>);
    static_assert(!ObjectLike<TestUnorderedMap>);
    static_assert(!ObjectLike<TestMap>);
    static_assert(!ObjectLike<TestOptional>);
    static_assert(!ObjectLike<TestUniquePtr>);
    static_assert(!ObjectLike<TestPointer>);
}

// ============================================================================
// SECTION 5: ParsableArrayLike - Must ONLY match array types
// ============================================================================

namespace test_array_concept {
    // Positive: arrays ARE ParsableArrayLike
    static_assert(ParsableArrayLike<std::array<int, 10>>);
    static_assert(ParsableArrayLike<std::array<bool, 5>>);
    static_assert(ParsableArrayLike<std::array<TestObject, 3>>);
    static_assert(ParsableArrayLike<std::array<std::array<int, 5>, 3>>);  // nested arrays
    static_assert(ParsableArrayLike<int[10]>);  // C array
    static_assert(ParsableArrayLike<bool[5]>);  // C array of bool
    static_assert(ParsableArrayLike<TestCArray>);  // C array type alias
    
    // Positive: arrays ARE SerializableArrayLike
    static_assert(SerializableArrayLike<std::array<int, 10>>);
    static_assert(SerializableArrayLike<std::array<bool, 5>>);
    static_assert(SerializableArrayLike<std::array<TestObject, 3>>);
    static_assert(SerializableArrayLike<int[10]>);  // C array
    static_assert(SerializableArrayLike<bool[5]>);  // C array of bool
    static_assert(SerializableArrayLike<TestCArray>);  // C array type alias
    
    // Negative: arrays are NOT other concepts
    static_assert(!BoolLike<TestArray>);
    static_assert(!BoolLike<TestCArray>);
    static_assert(!NumberLike<TestArray>);
    static_assert(!NumberLike<TestCArray>);
    static_assert(!StringLike<TestArray>);  // array<int>, not array<char>
    static_assert(!StringLike<TestCArray>);  // C array<int>, not array<char>
    static_assert(!ObjectLike<TestArray>);
    static_assert(!ObjectLike<TestCArray>);
    static_assert(!ParsableMapLike<TestArray>);
    static_assert(!ParsableMapLike<TestCArray>);
    static_assert(!JsonSerializableMap<TestArray>);
    static_assert(!JsonSerializableMap<TestCArray>);
    
    // CRITICAL: Arrays are NOT objects
    static_assert(!ObjectLike<TestArray>);
    static_assert(!ObjectLike<TestCArray>);
    
    // CRITICAL: Arrays are NOT maps
    static_assert(!ParsableMapLike<TestArray>);
    static_assert(!ParsableMapLike<TestCArray>);
    
    // Negative: Other types are NOT ParsableArrayLike
    static_assert(!ParsableArrayLike<bool>);
    static_assert(!ParsableArrayLike<int>);
    static_assert(!ParsableArrayLike<double>);
    static_assert(!ParsableArrayLike<TestString>);  // char array is string, not array
    static_assert(!ParsableArrayLike<TestObject>);
    static_assert(!ParsableArrayLike<TestCustomMap>);
    static_assert(!ParsableArrayLike<TestUnorderedMap>);
    static_assert(!ParsableArrayLike<TestMap>);
    static_assert(!ParsableArrayLike<TestOptional>);
    static_assert(!ParsableArrayLike<TestUniquePtr>);
    static_assert(!ParsableArrayLike<TestPointer>);
}

// ============================================================================
// SECTION 6: ParsableMapLike - Must ONLY match map types
// ============================================================================

namespace test_map_concept {
    // Positive: maps ARE ParsableMapLike
    static_assert(ParsableMapLike<TestCustomMap>);
    static_assert(ParsableMapLike<TestUnorderedMap>);
    static_assert(ParsableMapLike<TestMap>);
    
    struct AnotherMap {
        using key_type = std::array<char, 16>;
        using mapped_type = bool;
        
        constexpr auto try_emplace(key_type&& k, mapped_type&& v) {
            return std::pair{static_cast<const key_type*>(nullptr), true};
        }
        constexpr void clear() {}
    };
    static_assert(ParsableMapLike<AnotherMap>);
    
    // Map with struct value
    struct MapWithStructValue {
        using key_type = std::array<char, 32>;
        using mapped_type = TestObject;
        
        constexpr auto try_emplace(key_type&& k, mapped_type&& v) {
            return std::pair{static_cast<const key_type*>(nullptr), true};
        }
        constexpr void clear() {}
    };
    static_assert(ParsableMapLike<MapWithStructValue>);
    
    // Negative: maps are NOT other concepts
    static_assert(!BoolLike<TestCustomMap>);
    static_assert(!BoolLike<TestUnorderedMap>);
    static_assert(!BoolLike<TestMap>);
    static_assert(!NumberLike<TestCustomMap>);
    static_assert(!StringLike<TestCustomMap>);
    static_assert(!ObjectLike<TestCustomMap>);  // CRITICAL: Maps are NOT objects!
    static_assert(!ParsableArrayLike<TestCustomMap>);
    static_assert(!ParsableArrayLike<TestUnorderedMap>);
    static_assert(!ParsableArrayLike<TestMap>);
    static_assert(!SerializableArrayLike<TestCustomMap>);
    static_assert(!SerializableArrayLike<TestUnorderedMap>);
    static_assert(!SerializableArrayLike<TestMap>);
    
    // CRITICAL: Maps are NOT objects (this was the bug!)
    static_assert(!ObjectLike<TestCustomMap>);
    static_assert(!ObjectLike<TestUnorderedMap>);
    static_assert(!ObjectLike<TestMap>);
    static_assert(!ObjectLike<AnotherMap>);
    static_assert(!ObjectLike<MapWithStructValue>);
    
    // CRITICAL: Maps are NOT arrays
    static_assert(!ParsableArrayLike<TestCustomMap>);
    static_assert(!ParsableArrayLike<TestUnorderedMap>);
    static_assert(!ParsableArrayLike<TestMap>);
    // Negative: Other types are NOT ParsableMapLike
    static_assert(!ParsableMapLike<bool>);
    static_assert(!ParsableMapLike<int>);
    static_assert(!ParsableMapLike<double>);
    static_assert(!ParsableMapLike<TestString>);
    static_assert(!ParsableMapLike<TestObject>);  // Objects are not maps
    static_assert(!ParsableMapLike<TestArray>);
    static_assert(!ParsableMapLike<TestCArray>);
    static_assert(!ParsableMapLike<TestOptional>);
    static_assert(!ParsableMapLike<TestUniquePtr>);
    static_assert(!ParsableMapLike<TestPointer>);
    static_assert(!ParsableMapLike<TestUniquePtr>);
    
    // Map with invalid key type (not string or int)
    struct InvalidKeyMap {
        using key_type = float;  // Invalid!
        using mapped_type = int;
        
        constexpr auto try_emplace(key_type&& k, mapped_type&& v) {
            return std::pair{static_cast<const key_type*>(nullptr), true};
        }
        constexpr void clear() {}
    };
    
    // Structurally valid but not a JSON map (keys must be strings)
    static_assert(!ParsableMapLike<InvalidKeyMap>);
    static_assert(!ParsableValue<InvalidKeyMap>);  // Not parsable (invalid key type)
    static_assert(!ObjectLike<InvalidKeyMap>);  // Also not an object
    static_assert(!ParsableArrayLike<InvalidKeyMap>);  // Also not an array


     struct IntKeyMap {
        using key_type = std::size_t;
        using mapped_type = int;
        
        constexpr auto try_emplace(key_type&& k, mapped_type&& v) {
            return std::pair{static_cast<const key_type*>(nullptr), true};
        }
        constexpr void clear() {}
    };

    static_assert(ParsableMapLike<IntKeyMap>);
    static_assert(ParsableValue<IntKeyMap>);  // Not parsable (invalid key type)
    static_assert(!ObjectLike<IntKeyMap>);  // Also not an object
    static_assert(!ParsableArrayLike<IntKeyMap>);  // Also not an array

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
    static_assert(ParsableArrayLike<TestConsumer>);
    static_assert(ParsableValue<TestConsumer>);  // Highest level concept
    
    // ConsumingStreamerLike is NOT other concepts
    static_assert(!ObjectLike<TestConsumer>);
    static_assert(!ParsableMapLike<TestConsumer>);
    static_assert(!BoolLike<TestConsumer>);
    static_assert(!NumberLike<TestConsumer>);
    static_assert(!StringLike<TestConsumer>);
    
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
    static_assert(SerializableArrayLike<TestProducer>);
    static_assert(SerializableValue<TestProducer>);  // Highest level concept
    
    // ProducingStreamerLike is NOT other concepts
    static_assert(!ObjectLike<TestProducer>);
    static_assert(!JsonSerializableMap<TestProducer>);
    static_assert(!BoolLike<TestProducer>);
    static_assert(!NumberLike<TestProducer>);
    static_assert(!StringLike<TestProducer>);
    
    // Test with complex value_type
    struct Point { int x; int y; };
    
    using PointConsumer = SimpleConsumer<Point>;
    
    static_assert(ConsumingStreamerLike<PointConsumer>);
    static_assert(ParsableArrayLike<PointConsumer>);
    static_assert(!ObjectLike<PointConsumer>);
    
    // IMPORTANT: StreamerLike types use ARRAY interface, not MAP
    // Even though they have different extension point than cursor-based types
    static_assert(!ParsableMapLike<TestConsumer>);
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
    static_assert(ParsableMapLike<TestMapConsumer>);
    static_assert(ParsableValue<TestMapConsumer>);  // Highest level concept
    
    // ConsumingMapStreamerLike is NOT other concepts
    static_assert(!ObjectLike<TestMapConsumer>);
    static_assert(!ParsableArrayLike<TestMapConsumer>);
    static_assert(!BoolLike<TestMapConsumer>);
    static_assert(!NumberLike<TestMapConsumer>);
    static_assert(!StringLike<TestMapConsumer>);
    
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
    static_assert(SerializableValue<TestMapProducer>);  // Highest level concept
    
    // ProducingMapStreamerLike is NOT other concepts
    static_assert(!ObjectLike<TestMapProducer>);
    static_assert(!SerializableArrayLike<TestMapProducer>);
    static_assert(!BoolLike<TestMapProducer>);
    static_assert(!NumberLike<TestMapProducer>);
    static_assert(!StringLike<TestMapProducer>);
    
    // Test with struct value type
    struct Point { int x; int y; };
    
    using PointMapConsumer = SimpleMapConsumer<std::array<char, 16>, Point, 5>;
    
    static_assert(ConsumingMapStreamerLike<PointMapConsumer>);
    static_assert(ParsableMapLike<PointMapConsumer>);
    static_assert(!ObjectLike<PointMapConsumer>);
    static_assert(!ParsableArrayLike<PointMapConsumer>);
    
    // CRITICAL: Map streamers use MAP interface, not ARRAY
    static_assert(!ParsableArrayLike<TestMapConsumer>);
    static_assert(!SerializableArrayLike<TestMapProducer>);
    
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
    static_assert(!ParsableMapLike<InvalidMapConsumer>);
    
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
    
    static_assert(NullableParsableValue<std::optional<int>>);
    static_assert(!NonNullableParsableValue<std::optional<int>>);
    
    static_assert(NullableParsableValue<std::optional<bool>>);
    static_assert(NullableParsableValue<std::optional<TestObject>>);
    static_assert(NullableParsableValue<std::optional<TestArray>>);
    // static_assert(NullableParsableValue<std::optional<TestCArray>>); // illegal
    
    // Non-optionals are non-nullable
    static_assert(NonNullableParsableValue<int>);
    static_assert(NonNullableParsableValue<bool>);
    static_assert(NonNullableParsableValue<TestString>);
    static_assert(NonNullableParsableValue<TestObject>);
    static_assert(NonNullableParsableValue<TestArray>);
    static_assert(NonNullableParsableValue<TestCArray>);
    static_assert(NonNullableParsableValue<TestCustomMap>);
    static_assert(NonNullableParsableValue<TestUnorderedMap>);
    static_assert(NonNullableParsableValue<TestMap>);
    static_assert(!NullableParsableValue<int>);
    static_assert(!NullableParsableValue<bool>);
    static_assert(!NullableParsableValue<TestString>);
    static_assert(!NullableParsableValue<TestObject>);
    static_assert(!NullableParsableValue<TestArray>);
    static_assert(!NullableParsableValue<TestCArray>);
}

// ============================================================================
// SECTION 10: Comprehensive Type Classification Matrix
// ============================================================================

namespace test_classification_matrix {
    // Helper template to verify exactly one concept matches
    template<typename T>
    consteval int count_matching_concepts() {
        int count = 0;
        if constexpr (BoolLike<T>) count++;
        if constexpr (NumberLike<T>) count++;
        if constexpr (StringLike<T>) count++;
        if constexpr (ObjectLike<T>) count++;
        if constexpr (ParsableArrayLike<T>) count++;
        if constexpr (ParsableMapLike<T>) count++;
        return count;
    }
    
    // Each valid type should match EXACTLY ONE primary concept
    static_assert(count_matching_concepts<bool>() == 1, "bool should match exactly 1 concept");
    static_assert(count_matching_concepts<int>() == 1, "int should match exactly 1 concept");
    static_assert(count_matching_concepts<double>() == 1, "double should match exactly 1 concept");
    static_assert(count_matching_concepts<TestString>() == 1, "string should match exactly 1 concept");
    static_assert(count_matching_concepts<TestObject>() == 1, "object should match exactly 1 concept");
    static_assert(count_matching_concepts<TestArray>() == 1, "array should match exactly 1 concept");
    static_assert(count_matching_concepts<TestCArray>() == 1, "C array should match exactly 1 concept");
    static_assert(count_matching_concepts<TestCustomMap>() == 1, "map should match exactly 1 concept");
    static_assert(count_matching_concepts<TestUnorderedMap>() == 1, "unordered map should match exactly 1 concept");
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
    static_assert(ObjectLike<SingleFieldStruct>);
    static_assert(!ParsableMapLike<SingleFieldStruct>);
    static_assert(!ParsableArrayLike<SingleFieldStruct>);
    
    // Empty struct - still an object
    struct EmptyStruct {};
    static_assert(ObjectLike<EmptyStruct>);
    static_assert(!ParsableMapLike<EmptyStruct>);
    static_assert(!ParsableArrayLike<EmptyStruct>);
    
    // Struct with key_type and mapped_type but missing map interface
    // Should still be an OBJECT, not a MAP
    struct FakeMap {
        using key_type = int;
        using mapped_type = int;
        int x;
        int y;
    };
    // This doesn't have try_emplace/clear, so it's NOT a map
    static_assert(!ParsableMapLike<FakeMap>);
    static_assert(ObjectLike<FakeMap>);  // It's just a regular struct
    static_assert(ParsableValue<FakeMap>);  // Parsable as object (highest level)
    
    // Array of size 1
    using SingletonArray = std::array<int, 1>;
    static_assert(ParsableArrayLike<SingletonArray>);
    static_assert(!ObjectLike<SingletonArray>);
    static_assert(!ParsableMapLike<SingletonArray>);
    
    // Array of size 0
    using EmptyArray = std::array<int, 0>;
    static_assert(ParsableArrayLike<EmptyArray>);
    static_assert(!ObjectLike<EmptyArray>);
    static_assert(!ParsableMapLike<EmptyArray>);
    
    // Deeply nested types
    struct DeeplyNested {
        std::array<std::array<std::optional<TestObject>, 3>, 5> data;
    };
    static_assert(ObjectLike<DeeplyNested>);
    static_assert(!ParsableMapLike<DeeplyNested>);
    static_assert(!ParsableArrayLike<DeeplyNested>);
}

// ============================================================================
// SECTION 12: Test Annotated Types
// ============================================================================

namespace test_annotated_types {
    using namespace JsonFusion::options;
    
    // Annotated types should preserve their underlying concept
    using AnnotatedInt = Annotated<int, key<"mykey">>;
    static_assert(NumberLike<AnnotatedInt>);
    static_assert(!BoolLike<AnnotatedInt>);
    static_assert(!ObjectLike<AnnotatedInt>);
    
    using AnnotatedString = Annotated<std::array<char, 32>, key<"name">>;
    static_assert(StringLike<AnnotatedString>);
    static_assert(!NumberLike<AnnotatedString>);
    static_assert(!ObjectLike<AnnotatedString>);
    
    using AnnotatedArray = Annotated<std::array<int, 10>, key<"items">>;
    static_assert(ParsableArrayLike<AnnotatedArray>);
    static_assert(!ObjectLike<AnnotatedArray>);
    static_assert(!ParsableMapLike<AnnotatedArray>);
    
    using AnnotatedCArray = Annotated<int[10], key<"items">>;
    static_assert(ParsableArrayLike<AnnotatedCArray>);
    static_assert(SerializableArrayLike<AnnotatedCArray>);
    static_assert(!ObjectLike<AnnotatedCArray>);
    static_assert(!ParsableMapLike<AnnotatedCArray>);
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

