#include <JsonFusion/static_schema.hpp>
#include <JsonFusion/serializer.hpp>

using namespace JsonFusion;

// ===== Valid producing streamer (minimal implementation) =====
struct ValidProducingStreamer {
    using value_type = int;
    int current = 0;
    
    constexpr stream_read_result read(int& val) {
        if (current >= 5) return stream_read_result::end;
        val = current++;
        return stream_read_result::value;
    }
    constexpr void reset() { current = 0; }
};
static_assert(ProducingStreamerLike<ValidProducingStreamer>);

// ===== Producing streamer with struct value_type =====
struct Point { int x; int y; };

struct PointProducer {
    using value_type = Point;
    int index = 0;
    
    constexpr stream_read_result read(Point& p) {
        if (index >= 3) return stream_read_result::end;
        p = Point{index, index * 10};
        index++;
        return stream_read_result::value;
    }
    constexpr void reset() { index = 0; }
};
static_assert(ProducingStreamerLike<PointProducer>);

// ===== Producing streamer with string value_type =====
struct StringProducer {
    using value_type = std::string;
    int count = 0;
    
    constexpr stream_read_result read(std::string& str) {
        if (count >= 2) return stream_read_result::end;
        str = std::string(1, 'a' + count);
        count++;
        return stream_read_result::value;
    }
    constexpr void reset() { count = 0; }
};
static_assert(ProducingStreamerLike<StringProducer>);

// ===== Invalid: Missing value_type =====
struct MissingValueType {
    // No value_type!
    constexpr stream_read_result read(int& val) { return stream_read_result::end; }
    constexpr void reset() {}
};
static_assert(!ProducingStreamerLike<MissingValueType>);

// ===== Invalid: Missing read() =====
struct MissingRead {
    using value_type = int;
    // No read()!
    constexpr void reset() {}
};
static_assert(!ProducingStreamerLike<MissingRead>);

// ===== Invalid: Missing reset() =====
struct MissingReset {
    using value_type = int;
    constexpr stream_read_result read(int& val) { return stream_read_result::end; }
    // No reset()!
};
static_assert(!ProducingStreamerLike<MissingReset>);

// ===== Invalid: Wrong read() signature (takes by-value instead of reference) =====
struct WrongReadByValue {
    using value_type = int;
    constexpr stream_read_result read(int val) { return stream_read_result::end; }  // Takes by value - invalid
    constexpr void reset() {}
};
static_assert(!ProducingStreamerLike<WrongReadByValue>);

// ===== Invalid: Wrong read() return type =====
struct WrongReadReturn {
    using value_type = int;
    constexpr bool read(int& val) { return false; }  // Returns bool, not stream_read_result!
    constexpr void reset() {}
};
static_assert(!ProducingStreamerLike<WrongReadReturn>);

// ===== Invalid: Wrong reset() return type =====
struct WrongResetReturn {
    using value_type = int;
    constexpr stream_read_result read(int& val) { return stream_read_result::end; }
    constexpr bool reset() { return true; }  // Returns bool, not void!
};
static_assert(!ProducingStreamerLike<WrongResetReturn>);

// ===== Invalid: value_type is not SerializableValue =====
struct InvalidValueType {
    using value_type = int*;  // Pointers are not serializable!
    constexpr stream_read_result read(int*& val) { return stream_read_result::end; }
    constexpr void reset() {}
};
static_assert(!ProducingStreamerLike<InvalidValueType>);

// ===== Invalid: Non-aggregate value_type =====
struct NonAggregateValue {
    NonAggregateValue() {}  // Constructor
    int x;
};

struct NonAggregateProducer {
    using value_type = NonAggregateValue;
    constexpr stream_read_result read(NonAggregateValue& val) { 
        return stream_read_result::end; 
    }
    constexpr void reset() {}
};
static_assert(!ProducingStreamerLike<NonAggregateProducer>);

// ===== Primitives are not streamers =====
static_assert(!ProducingStreamerLike<int>);
static_assert(!ProducingStreamerLike<bool>);
static_assert(!ProducingStreamerLike<std::string>);

// ===== Structs without streamer methods are not streamers =====
struct RegularStruct {
    int x;
    bool flag;
};
static_assert(!ProducingStreamerLike<RegularStruct>);

// ===== ConsumingStreamer is not a ProducingStreamer =====
struct ConsumingStreamer {
    using value_type = int;
    constexpr bool consume(const int& val) { return true; }
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() {}
};
static_assert(!ProducingStreamerLike<ConsumingStreamer>);

