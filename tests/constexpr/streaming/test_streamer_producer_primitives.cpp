#include "../test_helpers.hpp"
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/static_schema.hpp>
#include <array>

using namespace TestHelpers;
using namespace JsonFusion;
using namespace JsonFusion::static_schema;

// ============================================================================
// Test: Producing Streamers for Primitives
// ============================================================================

// Simple integer producer - uses context for data
struct IntProducer {
    using value_type = int;
    
    mutable std::size_t index = 0;
    mutable const int* data_ptr = nullptr;  // From context
    mutable std::size_t data_count = 0;    // From context
    mutable int* sum_context = nullptr;     // For accumulating sum
    
    constexpr stream_read_result read(int& val) const {
        if (index >= data_count || !data_ptr) {
            return stream_read_result::end;
        }
        val = data_ptr[index++];
        // Use sum context if available
        if (sum_context) {
            (*sum_context) += val;
        }
        return stream_read_result::value;
    }
    
    constexpr void reset() const {
        index = 0;
    }
    
    // Context passing - receives data array and count
    struct DataContext {
        const int* data;
        std::size_t count;
    };
    
    constexpr void set_json_fusion_context(DataContext* ctx) const {
        if (ctx) {
            data_ptr = ctx->data;
            data_count = ctx->count;
        }
    }
};

static_assert(ProducingStreamerLike<IntProducer>);
static_assert(JsonSerializableArray<IntProducer>);

// Test 1: Producer as first-class type (direct serialization)
static_assert(
    []() constexpr {
        std::array<int, 3> data = {1, 2, 3};
        IntProducer producer{};
        IntProducer::DataContext ctx{data.data(), 3};
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx);
        // Should serialize as JSON array - check it contains the numbers
        return output.find("1") != std::string::npos
            && output.find("2") != std::string::npos
            && output.find("3") != std::string::npos;
    }(),
    "Producer works as first-class type (direct serialization)"
);

// Test 2: Producer works transparently in place of array (as struct field)
struct WithIntArray {
    std::array<int, 3> values;
};

struct WithIntProducer {
    IntProducer producer;
};

static_assert(
    []() constexpr {
        std::array<int, 3> data = {10, 20, 30};
        WithIntArray obj{};
        obj.values = data;
        std::string output1;
        JsonFusion::Serialize(obj, output1);
        
        WithIntProducer obj2{};
        IntProducer::DataContext ctx{data.data(), 3};
        std::string output2;
        JsonFusion::Serialize(obj2, output2, &ctx);
        
        // Both should produce similar JSON (array format)
        return output1.find("10") != std::string::npos 
            && output2.find("10") != std::string::npos;
    }(),
    "Producer works transparently in place of array (as struct field)"
);

// Test 3: Producer with context passing (data + sum accumulation)
struct ProducerContext {
    IntProducer::DataContext data_ctx;
    int sum = 0;
};

static_assert(
    []() constexpr {
        std::array<int, 3> data = {10, 20, 30};
        IntProducer producer{};
        ProducerContext ctx{{data.data(), 3}, 0};
        producer.sum_context = &ctx.sum;  // Set sum context before serialization
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx.data_ctx);
        // Sum context should have sum of all produced values
        return ctx.sum == 60;
    }(),
    "Producer with context passing (data + sum accumulation)"
);

// Test 4: Empty producer (no elements)
struct EmptyProducer {
    using value_type = int;
    
    constexpr stream_read_result read(int&) const {
        return stream_read_result::end;
    }
    
    constexpr void reset() const {}
};

static_assert(ProducingStreamerLike<EmptyProducer>);

static_assert(
    []() constexpr {
        EmptyProducer producer{};
        std::string output;
        JsonFusion::Serialize(producer, output);
        return output == "[]";
    }(),
    "Empty producer serializes to empty array"
);

// Test 5: Single element
static_assert(
    []() constexpr {
        std::array<int, 1> data = {42};
        IntProducer producer{};
        IntProducer::DataContext ctx{data.data(), 1};
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx);
        return output.find("42") != std::string::npos;
    }(),
    "Single element"
);

// Test 6: Many elements
static_assert(
    []() constexpr {
        std::array<int, 5> data = {1, 2, 3, 4, 5};
        IntProducer producer{};
        IntProducer::DataContext ctx{data.data(), 5};
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx);
        // Should contain all numbers
        return output.find("1") != std::string::npos
            && output.find("5") != std::string::npos;
    }(),
    "Many elements"
);

// Test 7: Boolean producer
struct BoolProducer {
    using value_type = bool;
    
    mutable std::size_t index = 0;
    mutable const bool* data_ptr = nullptr;
    mutable std::size_t data_count = 0;
    
    constexpr stream_read_result read(bool& val) const {
        if (index >= data_count || !data_ptr) {
            return stream_read_result::end;
        }
        val = data_ptr[index++];
        return stream_read_result::value;
    }
    
    constexpr void reset() const {
        index = 0;
    }
    
    struct DataContext {
        const bool* data;
        std::size_t count;
    };
    
    constexpr void set_json_fusion_context(DataContext* ctx) const {
        if (ctx) {
            data_ptr = ctx->data;
            data_count = ctx->count;
        }
    }
};

static_assert(ProducingStreamerLike<BoolProducer>);

static_assert(
    []() constexpr {
        std::array<bool, 3> data = {true, false, true};
        BoolProducer producer{};
        BoolProducer::DataContext ctx{data.data(), 3};
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx);
        return output.find("true") != std::string::npos
            && output.find("false") != std::string::npos;
    }(),
    "Boolean producer"
);

// Test 8: Producer returning stream_read_result::end
static_assert(
    []() constexpr {
        std::array<int, 2> data = {100, 200};
        IntProducer producer{};
        IntProducer::DataContext ctx{data.data(), 2};
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx);
        // After 2 elements, read() should return end
        return output.find("100") != std::string::npos
            && output.find("200") != std::string::npos;
    }(),
    "Producer returns end after all elements"
);

// Test 9: Producer in nested structures (transparency test)
struct Nested {
    struct Inner {
        IntProducer numbers;
    } inner;
};

static_assert(
    []() constexpr {
        std::array<int, 2> data = {100, 200};
        Nested obj{};
        IntProducer::DataContext ctx{data.data(), 2};
        std::string output;
        JsonFusion::Serialize(obj, output, &ctx);
        return output.find("100") != std::string::npos
            && output.find("200") != std::string::npos;
    }(),
    "Producer in nested structure"
);

// Test 10: Multiple producers in same struct
// Note: Each producer needs its own context set individually
struct MultipleProducers {
    IntProducer ints;
    BoolProducer bools;
};

static_assert(
    []() constexpr {
        std::array<int, 2> int_data = {1, 2};
        std::array<bool, 2> bool_data = {true, false};
        MultipleProducers obj{};
        // Set contexts individually before serialization
        IntProducer::DataContext int_ctx{int_data.data(), 2};
        BoolProducer::DataContext bool_ctx{bool_data.data(), 2};
        obj.ints.set_json_fusion_context(&int_ctx);
        obj.bools.set_json_fusion_context(&bool_ctx);
        std::string output;
        JsonFusion::Serialize(obj, output);
        return output.find("1") != std::string::npos
            && output.find("true") != std::string::npos;
    }(),
    "Multiple producers in same struct"
);

// Test 11: Producer with reset() - should restart from beginning
static_assert(
    []() constexpr {
        std::array<int, 2> data = {10, 20};
        IntProducer producer{};
        IntProducer::DataContext ctx{data.data(), 2};
        std::string output1;
        JsonFusion::Serialize(producer, output1, &ctx);
        
        producer.reset();
        std::string output2;
        JsonFusion::Serialize(producer, output2, &ctx);
        
        // Both should produce same output after reset
        return output1 == output2;
    }(),
    "Producer reset() restarts from beginning"
);

