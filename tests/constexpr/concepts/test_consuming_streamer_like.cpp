#include <JsonFusion/static_schema.hpp>
#include <JsonFusion/parser.hpp>

using namespace JsonFusion;

// ===== Valid consuming streamer (minimal implementation) =====
struct ValidConsumingStreamer {
    using value_type = int;
    
    constexpr bool consume(const int& val) { return true; }
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() {}
};
static_assert(ConsumingStreamerLike<ValidConsumingStreamer>);

// ===== Consuming streamer with struct value_type =====
struct Point { int x; int y; };

struct PointConsumer {
    using value_type = Point;
    int count = 0;
    
    constexpr bool consume(const Point& p) { 
        count++; 
        return true; 
    }
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() { count = 0; }
};
static_assert(ConsumingStreamerLike<PointConsumer>);

// ===== Consuming streamer with string value_type =====
struct StringConsumer {
    using value_type = std::string;
    
    constexpr bool consume(const std::string& str) { return true; }
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() {}
};
static_assert(ConsumingStreamerLike<StringConsumer>);

// ===== Invalid: Missing value_type =====
struct MissingValueType {
    // No value_type!
    constexpr bool consume(const int& val) { return true; }
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() {}
};
static_assert(!ConsumingStreamerLike<MissingValueType>);

// ===== Invalid: Missing consume() =====
struct MissingConsume {
    using value_type = int;
    // No consume()!
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() {}
};
static_assert(!ConsumingStreamerLike<MissingConsume>);

// ===== Invalid: Missing finalize() =====
struct MissingFinalize {
    using value_type = int;
    constexpr bool consume(const int& val) { return true; }
    // No finalize()!
    constexpr void reset() {}
};
static_assert(!ConsumingStreamerLike<MissingFinalize>);

// ===== Invalid: Missing reset() =====
struct MissingReset {
    using value_type = int;
    constexpr bool consume(const int& val) { return true; }
    constexpr bool finalize(bool success) { return success; }
    // No reset()!
};
static_assert(!ConsumingStreamerLike<MissingReset>);

// ===== Invalid: Wrong consume() signature (takes by-value instead of const ref) =====
struct WrongConsumeByValue {
    using value_type = int;
    constexpr bool consume(int val) { return true; }  // Takes by value - invalid
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() {}
};
static_assert(!ConsumingStreamerLike<WrongConsumeByValue>);

// ===== Invalid: Wrong consume() return type =====
struct WrongConsumeReturn {
    using value_type = int;
    constexpr void consume(const int& val) {}  // Returns void, not bool!
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() {}
};
static_assert(!ConsumingStreamerLike<WrongConsumeReturn>);

// ===== Invalid: Wrong finalize() return type =====
struct WrongFinalizeReturn {
    using value_type = int;
    constexpr bool consume(const int& val) { return true; }
    constexpr void finalize(bool success) {}  // Returns void, not bool!
    constexpr void reset() {}
};
static_assert(!ConsumingStreamerLike<WrongFinalizeReturn>);

// ===== Invalid: Wrong reset() return type =====
struct WrongResetReturn {
    using value_type = int;
    constexpr bool consume(const int& val) { return true; }
    constexpr bool finalize(bool success) { return success; }
    constexpr bool reset() { return true; }  // Returns bool, not void!
};
static_assert(!ConsumingStreamerLike<WrongResetReturn>);

// ===== Invalid: value_type is not ParsableValue =====
struct InvalidValueType {
    using value_type = int*;  // Pointers are not parsable!
    constexpr bool consume(const int*& val) { return true; }
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() {}
};
static_assert(!ConsumingStreamerLike<InvalidValueType>);

// ===== Invalid: Non-aggregate value_type =====
struct NonAggregateValue {
    NonAggregateValue() {}  // Constructor
    int x;
};

struct NonAggregateConsumer {
    using value_type = NonAggregateValue;
    constexpr bool consume(const NonAggregateValue& val) { return true; }
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() {}
};
static_assert(!ConsumingStreamerLike<NonAggregateConsumer>);

// ===== Primitives are not streamers =====
static_assert(!ConsumingStreamerLike<int>);
static_assert(!ConsumingStreamerLike<bool>);
static_assert(!ConsumingStreamerLike<std::string>);

// ===== Structs without streamer methods are not streamers =====
struct RegularStruct {
    int x;
    bool flag;
};
static_assert(!ConsumingStreamerLike<RegularStruct>);

