#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/static_schema.hpp>
#include <array>

using namespace TestHelpers;
using namespace JsonFusion;
using namespace JsonFusion::static_schema;

// ============================================================================
// Test: Consuming Streamers for Primitives
// ============================================================================

// Simple integer consumer with context
struct IntConsumer {
    using value_type = int;
    
    std::array<int, 10> items{};
    std::size_t count = 0;
    mutable int* context = nullptr;  // For context passing
    
    constexpr bool consume(const int& val) {
        if (count >= 10) return false;
        items[count++] = val;
        // Use context if available
        if (context) {
            (*context) += val;  // Sum values via context
        }
        return true;
    }
    
    constexpr bool finalize(bool success) {
        return success;
    }
    
    constexpr void reset() {
        count = 0;
    }
    
    // Context passing - high-level interface
    constexpr void set_jsonfusion_context(int* ctx) const {
        context = ctx;
    }
};

static_assert(ConsumingStreamerLike<IntConsumer>);

// Test 1: Consumer as first-class type (direct parsing)
static_assert(
    []() constexpr {
        IntConsumer consumer{};
        std::string json = R"([1, 2, 3])";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.count == 3
            && consumer.items[0] == 1
            && consumer.items[1] == 2
            && consumer.items[2] == 3;
    }(),
    "Consumer works as first-class type (direct parsing)"
);

// Test 2: Consumer works transparently in place of array (as struct field)
struct WithIntArray {
    std::array<int, 3> values;
};

struct WithIntConsumer {
    IntConsumer consumer;
};

static_assert(
    []() constexpr {
        WithIntArray obj{};
        std::string json = R"({"values": [1, 2, 3]})";
        auto result = JsonFusion::Parse(obj, json);
        if (!result) return false;
        return obj.values[0] == 1 && obj.values[1] == 2 && obj.values[2] == 3;
    }(),
    "Array works as baseline"
);

static_assert(
    []() constexpr {
        WithIntConsumer obj{};
        std::string json = R"({"consumer": [1, 2, 3]})";
        auto result = JsonFusion::Parse(obj, json);
        if (!result) return false;
        return obj.consumer.count == 3
            && obj.consumer.items[0] == 1
            && obj.consumer.items[1] == 2
            && obj.consumer.items[2] == 3;
    }(),
    "Consumer works transparently in place of array (as struct field)"
);

// Test 3: Consumer with context passing
static_assert(
    []() constexpr {
        IntConsumer consumer{};
        int context_sum = 0;
        std::string json = R"([10, 20, 30])";
        auto result = JsonFusion::Parse(consumer, json, &context_sum);
        if (!result) return false;
        // Context should have sum of all consumed values
        return context_sum == 60
            && consumer.count == 3
            && consumer.items[0] == 10
            && consumer.items[1] == 20
            && consumer.items[2] == 30;
    }(),
    "Consumer with context passing"
);

// Test 4: Boolean consumer
struct BoolConsumer {
    using value_type = bool;
    
    std::array<bool, 5> items{};
    std::size_t count = 0;
    
    constexpr bool consume(const bool& val) {
        if (count >= 5) return false;
        items[count++] = val;
        return true;
    }
    
    constexpr bool finalize(bool success) {
        return success;
    }
    
    constexpr void reset() {
        count = 0;
    }
};

static_assert(ConsumingStreamerLike<BoolConsumer>);

static_assert(
    []() constexpr {
        BoolConsumer consumer{};
        std::string json = R"([true, false, true])";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.count == 3
            && consumer.items[0] == true
            && consumer.items[1] == false
            && consumer.items[2] == true;
    }(),
    "Boolean consumer"
);

// Test 5: Char consumer (for char arrays)
struct CharConsumer {
    using value_type = char;
    
    std::array<char, 10> items{};
    std::size_t count = 0;
    
    constexpr bool consume(const char& val) {
        if (count >= 10) return false;
        items[count++] = val;
        return true;
    }
    
    constexpr bool finalize(bool success) {
        return success;
    }
    
    constexpr void reset() {
        count = 0;
    }
};

static_assert(ConsumingStreamerLike<CharConsumer>);

static_assert(
    []() constexpr {
        CharConsumer consumer{};
        // JSON array of character codes: [65, 66, 67] = "ABC"
        std::string json = R"([65, 66, 67])";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.count == 3
            && consumer.items[0] == 65  // 'A'
            && consumer.items[1] == 66  // 'B'
            && consumer.items[2] == 67; // 'C'
    }(),
    "Char consumer"
);

// Test 6: Empty array
static_assert(
    []() constexpr {
        IntConsumer consumer{};
        std::string json = R"([])";
        auto result = JsonFusion::Parse(consumer, json);
        return result && consumer.count == 0;
    }(),
    "Empty array parsing succeeds"
);

// Test 7: Single element
static_assert(
    []() constexpr {
        IntConsumer consumer{};
        std::string json = R"([42])";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.count == 1 && consumer.items[0] == 42;
    }(),
    "Single element"
);

// Test 8: Early termination (returning false from consume())
struct LimitedConsumer {
    using value_type = int;
    
    std::array<int, 5> items{};
    std::size_t count = 0;
    std::size_t max_count = 2;  // Stop after 2 elements
    
    constexpr bool consume(const int& val) {
        if (count >= max_count) return false;  // Early termination
        items[count++] = val;
        return true;
    }
    
    constexpr bool finalize(bool success) {
        // Should receive false because consume() returned false
        return !success;  // Expect failure due to early termination
    }
    
    constexpr void reset() {
        count = 0;
    }
};

static_assert(ConsumingStreamerLike<LimitedConsumer>);

static_assert(
    []() constexpr {
        LimitedConsumer consumer{};
        std::string json = R"([1, 2, 3, 4, 5])";
        auto result = JsonFusion::Parse(consumer, json);
        // Should fail because consume() returned false after 2 elements
        return !result && consumer.count == 2;
    }(),
    "Early termination via consume() returning false"
);

// Test 9: Element validation in consume()
struct ValidatingConsumer {
    using value_type = int;
    
    std::array<int, 10> items{};
    std::size_t count = 0;
    
    constexpr bool consume(const int& val) {
        // Validate: only accept positive values
        if (val <= 0) return false;
        if (count >= 10) return false;
        items[count++] = val;
        return true;
    }
    
    constexpr bool finalize(bool success) {
        return success;
    }
    
    constexpr void reset() {
        count = 0;
    }
};

static_assert(ConsumingStreamerLike<ValidatingConsumer>);

static_assert(
    []() constexpr {
        ValidatingConsumer consumer{};
        std::string json = R"([1, 2, 3])";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.count == 3;
    }(),
    "Validation: accept valid values"
);

static_assert(
    []() constexpr {
        ValidatingConsumer consumer{};
        std::string json = R"([1, -5, 3])";
        auto result = JsonFusion::Parse(consumer, json);
        // Should fail because -5 is rejected by consume()
        return !result && consumer.count == 1;  // Only first element consumed
    }(),
    "Validation: reject invalid values"
);

// Test 10: Consumer works in nested structures (transparency test)
struct Nested {
    struct Inner {
        IntConsumer numbers;
    } inner;
};

static_assert(
    []() constexpr {
        Nested obj{};
        std::string json = R"({"inner": {"numbers": [100, 200]}})";
        auto result = JsonFusion::Parse(obj, json);
        if (!result) return false;
        return obj.inner.numbers.count == 2
            && obj.inner.numbers.items[0] == 100
            && obj.inner.numbers.items[1] == 200;
    }(),
    "Consumer in nested structure"
);

// Test 10: Multiple consumers in same struct
struct MultipleConsumers {
    IntConsumer ints;
    BoolConsumer bools;
};

static_assert(
    []() constexpr {
        MultipleConsumers obj{};
        std::string json = R"({"ints": [1, 2], "bools": [true, false]})";
        auto result = JsonFusion::Parse(obj, json);
        if (!result) return false;
        return obj.ints.count == 2
            && obj.ints.items[0] == 1
            && obj.ints.items[1] == 2
            && obj.bools.count == 2
            && obj.bools.items[0] == true
            && obj.bools.items[1] == false;
    }(),
    "Multiple consumers in same struct"
);

