#include "../test_helpers.hpp"
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/static_schema.hpp>
#include <array>

using namespace TestHelpers;
using namespace JsonFusion;
using namespace JsonFusion::static_schema;

// ============================================================================
// Test: Producing Map Streamers
// ============================================================================

// Map entry structure required by ProducingMapStreamerLike
template<typename K, typename V>
struct MapEntry {
    K key;
    V value;
};

// Simple map producer - uses context for data
struct IntMapProducer {
    using value_type = MapEntry<std::array<char, 32>, int>;
    
    mutable std::size_t index = 0;
    mutable const value_type* entries_ptr = nullptr;  // From context
    mutable std::size_t entries_count = 0;           // From context
    mutable int* sum_context = nullptr;              // For accumulating sum
    
    constexpr stream_read_result read(value_type& entry) const {
        if (index >= entries_count || !entries_ptr) {
            return stream_read_result::end;
        }
        entry = entries_ptr[index++];
        // Use sum context if available
        if (sum_context) {
            (*sum_context) += entry.value;
        }
        return stream_read_result::value;
    }
    
    constexpr void reset() const {
        index = 0;
    }
    
    // Context passing - receives entries array and count
    struct DataContext {
        const value_type* entries;
        std::size_t count;
    };
    
    constexpr void set_jsonfusion_context(DataContext* ctx) const {
        if (ctx) {
            entries_ptr = ctx->entries;
            entries_count = ctx->count;
        }
    }
};

static_assert(ProducingMapStreamerLike<IntMapProducer>);
static_assert(JsonSerializableMap<IntMapProducer>);

// Test 1: Map producer as first-class type (direct serialization)
static_assert(
    []() constexpr {
        std::array<IntMapProducer::value_type, 3> entries = {{
            {{'k','e','y','1','\0'}, 10},
            {{'k','e','y','2','\0'}, 20},
            {{'k','e','y','3','\0'}, 30}
        }};
        IntMapProducer producer{};
        IntMapProducer::DataContext ctx{entries.data(), 3};
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx);
        // Should serialize as JSON object - check it contains the keys
        return output.find("key1") != std::string::npos
            && output.find("key2") != std::string::npos
            && output.find("key3") != std::string::npos;
    }(),
    "Map producer works as first-class type (direct serialization)"
);

// Test 2: Map producer works transparently in place of map (as struct field)
struct WithIntMapProducer {
    IntMapProducer producer;
};

static_assert(
    []() constexpr {
        std::array<IntMapProducer::value_type, 2> entries = {{
            {{'a','\0'}, 10},
            {{'b','\0'}, 20}
        }};
        WithIntMapProducer obj{};
        IntMapProducer::DataContext ctx{entries.data(), 2};
        std::string output;
        JsonFusion::Serialize(obj, output, &ctx);
        return output.find("\"a\"") != std::string::npos
            && output.find("\"b\"") != std::string::npos;
    }(),
    "Map producer works transparently in place of map (as struct field)"
);

// Test 3: Map producer with context passing (data + sum accumulation)
struct MapProducerContext {
    IntMapProducer::DataContext data_ctx;
    int sum = 0;
};

static_assert(
    []() constexpr {
        std::array<IntMapProducer::value_type, 3> entries = {{
            {{'a','\0'}, 10},
            {{'b','\0'}, 20},
            {{'c','\0'}, 30}
        }};
        IntMapProducer producer{};
        MapProducerContext ctx{{entries.data(), 3}, 0};
        producer.sum_context = &ctx.sum;  // Set sum context before serialization
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx.data_ctx);
        // Sum context should have sum of all produced values
        return ctx.sum == 60;
    }(),
    "Map producer with context passing (data + sum accumulation)"
);

// Test 4: Empty map producer
struct EmptyMapProducer {
    using value_type = MapEntry<std::array<char, 32>, int>;
    
    constexpr stream_read_result read(value_type&) const {
        return stream_read_result::end;
    }
    
    constexpr void reset() const {}
};

static_assert(ProducingMapStreamerLike<EmptyMapProducer>);

static_assert(
    []() constexpr {
        EmptyMapProducer producer{};
        std::string output;
        JsonFusion::Serialize(producer, output);
        return output == "{}";
    }(),
    "Empty map producer serializes to empty object"
);

// Test 5: Single entry
static_assert(
    []() constexpr {
        std::array<IntMapProducer::value_type, 1> entries = {{
            {{'s','i','n','g','l','e','\0'}, 42}
        }};
        IntMapProducer producer{};
        IntMapProducer::DataContext ctx{entries.data(), 1};
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx);
        return output.find("single") != std::string::npos
            && output.find("42") != std::string::npos;
    }(),
    "Single entry"
);

// Test 6: Many entries
static_assert(
    []() constexpr {
        std::array<IntMapProducer::value_type, 5> entries = {{
            {{'a','\0'}, 1},
            {{'b','\0'}, 2},
            {{'c','\0'}, 3},
            {{'d','\0'}, 4},
            {{'e','\0'}, 5}
        }};
        IntMapProducer producer{};
        IntMapProducer::DataContext ctx{entries.data(), 5};
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx);
        // Should contain all keys
        return output.find("\"a\"") != std::string::npos
            && output.find("\"e\"") != std::string::npos;
    }(),
    "Many entries"
);

// Test 7: Map producer with string values
struct StringMapProducer {
    using value_type = MapEntry<std::array<char, 32>, std::string>;
    
    mutable std::size_t index = 0;
    mutable const value_type* entries_ptr = nullptr;
    mutable std::size_t entries_count = 0;
    
    constexpr stream_read_result read(value_type& entry) const {
        if (index >= entries_count || !entries_ptr) {
            return stream_read_result::end;
        }
        entry = entries_ptr[index++];
        return stream_read_result::value;
    }
    
    constexpr void reset() const {
        index = 0;
    }
    
    struct DataContext {
        const value_type* entries;
        std::size_t count;
    };
    
    constexpr void set_jsonfusion_context(DataContext* ctx) const {
        if (ctx) {
            entries_ptr = ctx->entries;
            entries_count = ctx->count;
        }
    }
};

static_assert(ProducingMapStreamerLike<StringMapProducer>);

static_assert(
    []() constexpr {
        std::array<StringMapProducer::value_type, 2> entries = {{
            {{{'n','a','m','e','\0'}}, std::string("Alice")},
            {{{'c','i','t','y','\0'}}, std::string("NYC")}
        }};
        StringMapProducer producer{};
        StringMapProducer::DataContext ctx{entries.data(), 2};
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx);
        return output.find("name") != std::string::npos
            && output.find("Alice") != std::string::npos;
    }(),
    "Map producer with string values"
);

// Test 8: Map producer with boolean values
struct BoolMapProducer {
    using value_type = MapEntry<std::array<char, 32>, bool>;
    
    mutable std::size_t index = 0;
    mutable const value_type* entries_ptr = nullptr;
    mutable std::size_t entries_count = 0;
    
    constexpr stream_read_result read(value_type& entry) const {
        if (index >= entries_count || !entries_ptr) {
            return stream_read_result::end;
        }
        entry = entries_ptr[index++];
        return stream_read_result::value;
    }
    
    constexpr void reset() const {
        index = 0;
    }
    
    struct DataContext {
        const value_type* entries;
        std::size_t count;
    };
    
    constexpr void set_jsonfusion_context(DataContext* ctx) const {
        if (ctx) {
            entries_ptr = ctx->entries;
            entries_count = ctx->count;
        }
    }
};

static_assert(ProducingMapStreamerLike<BoolMapProducer>);

static_assert(
    []() constexpr {
        std::array<BoolMapProducer::value_type, 2> entries = {{
            {{{'f','l','a','g','1','\0'}}, true},
            {{{'f','l','a','g','2','\0'}}, false}
        }};
        BoolMapProducer producer{};
        BoolMapProducer::DataContext ctx{entries.data(), 2};
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx);
        return output.find("flag1") != std::string::npos
            && output.find("true") != std::string::npos
            && output.find("false") != std::string::npos;
    }(),
    "Map producer with boolean values"
);

// Test 9: Map producer in nested structures (transparency test)
struct Nested {
    struct Inner {
        IntMapProducer map_producer;
    } inner;
};

static_assert(
    []() constexpr {
        std::array<IntMapProducer::value_type, 2> entries = {{
            {{{'x','\0'}}, 100},
            {{{'y','\0'}}, 200}
        }};
        Nested obj{};
        IntMapProducer::DataContext ctx{entries.data(), 2};
        std::string output;
        JsonFusion::Serialize(obj, output, &ctx);
        return output.find("x") != std::string::npos
            && output.find("y") != std::string::npos;
    }(),
    "Map producer in nested structure"
);

// Test 10: Producer returning stream_read_result::end
static_assert(
    []() constexpr {
        std::array<IntMapProducer::value_type, 2> entries = {{
            {{{'a','\0'}}, 100},
            {{{'b','\0'}}, 200}
        }};
        IntMapProducer producer{};
        IntMapProducer::DataContext ctx{entries.data(), 2};
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx);
        // After 2 entries, read() should return end
        return output.find("\"a\"") != std::string::npos
            && output.find("\"b\"") != std::string::npos;
    }(),
    "Map producer returns end after all entries"
);

// Test 11: Producer with reset() - should restart from beginning
static_assert(
    []() constexpr {
        std::array<IntMapProducer::value_type, 2> entries = {{
            {{{'a','\0'}}, 10},
            {{{'b','\0'}}, 20}
        }};
        IntMapProducer producer{};
        IntMapProducer::DataContext ctx{entries.data(), 2};
        std::string output1;
        JsonFusion::Serialize(producer, output1, &ctx);
        
        producer.reset();
        std::string output2;
        JsonFusion::Serialize(producer, output2, &ctx);
        
        // Both should produce same output after reset
        return output1 == output2;
    }(),
    "Map producer reset() restarts from beginning"
);

// ============================================================================
// Tests with std::string keys (instead of std::array<char, N>)
// ============================================================================

// Test 12: Map producer with std::string keys - basic functionality
struct StringKeyMapProducer {
    using value_type = MapEntry<std::string, int>;
    
    mutable std::size_t index = 0;
    mutable const value_type* entries_ptr = nullptr;
    mutable std::size_t entries_count = 0;
    
    constexpr stream_read_result read(value_type& entry) const {
        if (index >= entries_count || !entries_ptr) {
            return stream_read_result::end;
        }
        entry = entries_ptr[index++];
        return stream_read_result::value;
    }
    
    constexpr void reset() const {
        index = 0;
    }
    
    struct DataContext {
        const value_type* entries;
        std::size_t count;
    };
    
    constexpr void set_jsonfusion_context(DataContext* ctx) const {
        if (ctx) {
            entries_ptr = ctx->entries;
            entries_count = ctx->count;
        }
    }
};

static_assert(ProducingMapStreamerLike<StringKeyMapProducer>);

static_assert(
    []() constexpr {
        std::array<StringKeyMapProducer::value_type, 3> entries = {{
            {std::string("first_key"), 10},
            {std::string("second_key"), 20},
            {std::string("third_key"), 30}
        }};
        StringKeyMapProducer producer{};
        StringKeyMapProducer::DataContext ctx{entries.data(), 3};
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx);
        // Should serialize as JSON object - check it contains the keys
        return output.find("first_key") != std::string::npos
            && output.find("second_key") != std::string::npos
            && output.find("third_key") != std::string::npos;
    }(),
    "Map producer with std::string keys - basic functionality"
);

// Test 13: Map producer with std::string keys - many entries
static_assert(
    []() constexpr {
        std::array<StringKeyMapProducer::value_type, 5> entries = {{
            {std::string("alpha"), 1},
            {std::string("beta"), 2},
            {std::string("gamma"), 3},
            {std::string("delta"), 4},
            {std::string("epsilon"), 5}
        }};
        StringKeyMapProducer producer{};
        StringKeyMapProducer::DataContext ctx{entries.data(), 5};
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx);
        // Should contain all keys
        return output.find("\"alpha\"") != std::string::npos
            && output.find("\"epsilon\"") != std::string::npos;
    }(),
    "Map producer with std::string keys - many entries"
);

// Test 14: Map producer with std::string keys - context passing
struct StringKeyMapProducerContext {
    StringKeyMapProducer::DataContext data_ctx;
    int sum = 0;
};

static_assert(
    []() constexpr {
        std::array<StringKeyMapProducer::value_type, 3> entries = {{
            {std::string("a"), 10},
            {std::string("b"), 20},
            {std::string("c"), 30}
        }};
        StringKeyMapProducer producer{};
        StringKeyMapProducerContext ctx{{entries.data(), 3}, 0};
        // Note: sum accumulation would require additional context setup
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx.data_ctx);
        // At minimum, serialization should succeed with context
        return output.find("\"a\"") != std::string::npos
            && output.find("\"c\"") != std::string::npos;
    }(),
    "Map producer with std::string keys - context passing"
);

