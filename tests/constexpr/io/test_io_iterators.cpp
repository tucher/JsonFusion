#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <string_view>
#include <array>
#include <optional>

// ============================================================================
// Strict Single-Pass Input Iterator with End Detection
// ============================================================================

/// A strict single-pass input iterator that enforces InputIterator semantics.
///
/// Key properties enforced:
/// 1. Advancing ANY copy invalidates ALL other copies (simulates istreambuf_iterator)
/// 2. Cannot dereference an invalidated iterator
/// 3. No random access, no pointer arithmetic
/// 4. Tracks end position for proper termination detection
///
/// This catches bugs like the peek-ahead issue in leading zero validation,
/// where copying an iterator and advancing the copy would corrupt stream position.
template<typename Container>
class SinglePassIteratorWithEnd {
    const Container* container;
    std::size_t* shared_position;
    std::size_t end_position;
    std::size_t my_snapshot;

public:
    using iterator_category = std::input_iterator_tag;
    using value_type = char;
    using difference_type = std::ptrdiff_t;
    using pointer = const char*;
    using reference = char;

    constexpr SinglePassIteratorWithEnd()
        : container(nullptr), shared_position(nullptr), end_position(0), my_snapshot(0) {}

    constexpr SinglePassIteratorWithEnd(const Container& c, std::size_t* pos, std::size_t end_pos)
        : container(&c), shared_position(pos), end_position(end_pos), my_snapshot(*pos) {}

    constexpr SinglePassIteratorWithEnd(const SinglePassIteratorWithEnd& other)
        : container(other.container)
        , shared_position(other.shared_position)
        , end_position(other.end_position)
        , my_snapshot(other.shared_position ? *other.shared_position : 0) {}

    constexpr SinglePassIteratorWithEnd& operator=(const SinglePassIteratorWithEnd& other) {
        container = other.container;
        shared_position = other.shared_position;
        end_position = other.end_position;
        my_snapshot = other.shared_position ? *other.shared_position : 0;
        return *this;
    }

    constexpr SinglePassIteratorWithEnd& operator++() {
        if (my_snapshot != *shared_position) {
            container = nullptr;  // Poison - multi-pass violation
        }
        ++(*shared_position);
        ++my_snapshot;
        return *this;
    }

    constexpr SinglePassIteratorWithEnd operator++(int) {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    constexpr char operator*() const {
        if (!container || !shared_position || my_snapshot != *shared_position) {
            return '\0';
        }
        return (*container)[*shared_position];
    }

    constexpr bool operator==(const SinglePassIteratorWithEnd& other) const {
        // Check if we've reached the end
        if (shared_position && *shared_position >= end_position) {
            // At end
            if (!other.shared_position || (other.shared_position && *other.shared_position >= other.end_position)) {
                return true;  // Both at end
            }
        }
        if (!shared_position && other.shared_position && *other.shared_position >= other.end_position) {
            return true;  // This is end sentinel, other is at end
        }
        if (shared_position && !other.shared_position && *shared_position >= end_position) {
            return true;  // Other is end sentinel, this is at end
        }
        if (!shared_position && !other.shared_position) {
            return true;  // Both are end sentinels
        }
        return false;
    }

    // Delete range operations
    constexpr SinglePassIteratorWithEnd operator+(difference_type) const = delete;
    constexpr SinglePassIteratorWithEnd operator-(difference_type) const = delete;
    constexpr difference_type operator-(const SinglePassIteratorWithEnd&) const = delete;
    constexpr SinglePassIteratorWithEnd& operator+=(difference_type) = delete;
    constexpr SinglePassIteratorWithEnd& operator-=(difference_type) = delete;
    constexpr char operator[](difference_type) const = delete;
    constexpr bool operator<(const SinglePassIteratorWithEnd&) const = delete;
    constexpr bool operator>(const SinglePassIteratorWithEnd&) const = delete;
    constexpr bool operator<=(const SinglePassIteratorWithEnd&) const = delete;
    constexpr bool operator>=(const SinglePassIteratorWithEnd&) const = delete;
};

// ============================================================================
// Custom Output Iterator - Blocks Range Operations
// ============================================================================

/// A true output iterator that can ONLY write one byte at a time.
/// This proves the serializer doesn't require pointer arithmetic.
template<typename Container>
class ByteByByteOutputIterator {
    Container* container;
    std::size_t index;

public:
    // Iterator traits
    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = void;

    constexpr ByteByByteOutputIterator() : container(nullptr), index(0) {}
    
    constexpr ByteByByteOutputIterator(Container& c, std::size_t idx = 0)
        : container(&c), index(idx) {}

    constexpr ByteByByteOutputIterator& operator++() {
        ++index;
        return *this;
    }

    constexpr ByteByByteOutputIterator operator++(int) {
        auto temp = *this;
        ++index;
        return temp;
    }

    constexpr ByteByByteOutputIterator& operator=(char ch) {
        (*container)[index] = ch;
        return *this;
    }

    constexpr ByteByByteOutputIterator& operator*() {
        return *this;
    }

    constexpr std::size_t position() const {
        return index;
    }

    constexpr bool operator==(const ByteByByteOutputIterator& other) const {
        return container == other.container && index == other.index;
    }

    // Explicitly DELETE range operations to prove they're not used
    constexpr ByteByByteOutputIterator operator+(difference_type) const = delete;
    constexpr ByteByByteOutputIterator operator-(difference_type) const = delete;
    constexpr difference_type operator-(const ByteByByteOutputIterator&) const = delete;
    constexpr ByteByByteOutputIterator& operator+=(difference_type) = delete;
    constexpr ByteByByteOutputIterator& operator-=(difference_type) = delete;
};

// ============================================================================
// Tests
// ============================================================================

// Test: Parse primitive types byte-by-byte
static_assert([]() constexpr {
    struct Simple {
        int value;
        bool flag;
    };

    std::string json = R"({"value": 42, "flag": true})";
    std::size_t pos = 0;
    
    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end;
    
    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end);
    
    return result && obj.value == 42 && obj.flag == true;
}());

// Test: Parse nested structures byte-by-byte
static_assert([]() constexpr {
    struct Nested {
        int x;
        int y;
    };
    struct Outer {
        std::string name;
        Nested nested;
    };

    std::string json = R"({"name": "test", "nested": {"x": 10, "y": 20}})";
    std::size_t pos = 0;
    
    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end;
    
    Outer obj{};
    auto result = JsonFusion::Parse(obj, begin, end);
    
    return result 
        && obj.name == "test" 
        && obj.nested.x == 10 
        && obj.nested.y == 20;
}());

// Test: Parse arrays byte-by-byte
static_assert([]() constexpr {
    struct Config {
        std::vector<int> numbers;
        std::array<bool, 3> flags;
    };

    std::string json = R"({"numbers": [1,2,3,4], "flags": [true, false, true]})";
    std::size_t pos = 0;
    
    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end;
    
    Config obj{};
    auto result = JsonFusion::Parse(obj, begin, end);
    
    return result 
        && obj.numbers.size() == 4
        && obj.numbers[2] == 3
        && obj.flags[0] == true
        && obj.flags[1] == false;
}());

// Test: Parse optionals byte-by-byte
static_assert([]() constexpr {
    struct Config {
        std::optional<int> present;
        std::optional<int> absent;
        std::optional<std::string> text;
    };

    std::string json = R"({"present": 100, "absent": null, "text": "hello"})";
    std::size_t pos = 0;
    
    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end;
    
    Config obj{};
    auto result = JsonFusion::Parse(obj, begin, end);
    
    return result 
        && obj.present.has_value()
        && *obj.present == 100
        && !obj.absent.has_value()
        && obj.text == "hello";
}());

// Test: Serialize byte-by-byte
static_assert([]() constexpr {
    struct Simple {
        int value;
        bool flag;
        std::string text;
    };

    Simple obj{42, true, "test"};
    std::array<char, 256> buffer{};
    
    ByteByByteOutputIterator<std::array<char, 256>> out(buffer, 0);
    ByteByByteOutputIterator<std::array<char, 256>> end_sentinel(buffer, buffer.size());
    
    auto result = JsonFusion::Serialize(obj, out, end_sentinel);
    // Verify output
    std::string_view expected = R"({"value":42,"flag":true,"text":"test"})";
    std::string_view actual(buffer.data(), result.bytesWritten());
    
    return !!result && actual == expected;
}());

// Test: Complex nested structure byte-by-byte
static_assert([]() constexpr {
    struct Point { int x; int y; };
    struct Config {
        std::string name;
        std::vector<Point> points;
        std::optional<std::vector<int>> opt_vec;
    };

    std::string json = R"({
        "name": "path",
        "points": [{"x": 1, "y": 2}, {"x": 3, "y": 4}],
        "opt_vec": [10, 20, 30]
    })";
    std::size_t pos = 0;
    
    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end;
    
    Config obj{};
    auto result = JsonFusion::Parse(obj, begin, end);
    
    return result 
        && obj.name == "path"
        && obj.points.size() == 2
        && obj.points[0].x == 1
        && obj.points[1].y == 4
        && obj.opt_vec.has_value()
        && obj.opt_vec->size() == 3
        && (*obj.opt_vec)[2] == 30;
}());

// Test: Error detection with byte-by-byte parsing
static_assert([]() constexpr {
    struct Simple { int value; };

    std::string json = R"({"value": "not_a_number"})";  // String where int expected
    std::size_t pos = 0;
    
    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end;
    
    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end);
    
    // Should fail with parse error
    return !result && result.readerError() == JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER;
}());

// Test: Whitespace handling with byte-by-byte parsing
static_assert([]() constexpr {
    struct Simple { int value; };

    std::string json = R"(  {  "value"  :  123  }  )";
    std::size_t pos = 0;
    
    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end;
    
    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end);
    
    return result && obj.value == 123;
}());

// Test: String escapes with byte-by-byte parsing
static_assert([]() constexpr {
    struct Simple { std::string text; };

    std::string json = R"({"text": "hello\nworld\t\u0041"})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end;

    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end);

    return result
        && obj.text.size() == 13  // "hello\nworld\tA"
        && obj.text[5] == '\n'
        && obj.text[11] == '\t'
        && obj.text[12] == 'A';
}());

// ============================================================================
// Tests for Issue #4: Zero-Starting Numbers with Single-Pass Iterators
// ============================================================================
// These tests verify that the leading zero check (RFC 8259) works correctly
// without peek-ahead, which would break single-pass iterators like istreambuf_iterator.

// Test: Zero integer value (the exact trigger for the old peek-ahead bug)
static_assert([]() constexpr {
    struct Simple { int value; };

    std::string json = R"({"value": 0})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    return result && obj.value == 0;
}());

// Test: Zero in array (triggers leading zero check for each element)
static_assert([]() constexpr {
    struct Config { std::vector<int> nums; };

    std::string json = R"({"nums": [0]})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Config obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    return result && obj.nums.size() == 1 && obj.nums[0] == 0;
}());

// Test: Multiple zeros in array
static_assert([]() constexpr {
    struct Config { std::vector<int> nums; };

    std::string json = R"({"nums": [0, 1, 0, 2, 0]})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Config obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    return result
        && obj.nums.size() == 5
        && obj.nums[0] == 0
        && obj.nums[1] == 1
        && obj.nums[2] == 0
        && obj.nums[3] == 2
        && obj.nums[4] == 0;
}());

// Test: Negative zero
static_assert([]() constexpr {
    struct Simple { double value; };

    std::string json = R"({"value": -0})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    return result && obj.value == 0.0;
}());

// ============================================================================
// Tests for Floating-Point Numbers with Single-Pass Iterators
// ============================================================================

// Test: 0.5 - This was corrupted to 5 in the old buggy code!
static_assert([]() constexpr {
    struct Simple { double value; };

    std::string json = R"({"value": 0.5})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    // Must be 0.5, not 5!
    return result && obj.value > 0.4 && obj.value < 0.6;
}());

// Test: -0.5
static_assert([]() constexpr {
    struct Simple { double value; };

    std::string json = R"({"value": -0.5})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    return result && obj.value > -0.6 && obj.value < -0.4;
}());

// Test: 0.123456 - Multiple digits after zero
static_assert([]() constexpr {
    struct Simple { double value; };

    std::string json = R"({"value": 0.123456})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    return result && obj.value > 0.123 && obj.value < 0.124;
}());

// Test: 0e5 - Zero with exponent
static_assert([]() constexpr {
    struct Simple { double value; };

    std::string json = R"({"value": 0e5})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    return result && obj.value == 0.0;
}());

// Test: 0.0e10 - Zero decimal with exponent
static_assert([]() constexpr {
    struct Simple { double value; };

    std::string json = R"({"value": 0.0e10})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    return result && obj.value == 0.0;
}());

// Test: Regular floating point (non-zero starting)
static_assert([]() constexpr {
    struct Simple { double value; };

    std::string json = R"({"value": 3.14159})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    return result && obj.value > 3.14 && obj.value < 3.15;
}());

// Test: Scientific notation
static_assert([]() constexpr {
    struct Simple { double value; };

    std::string json = R"({"value": 1.5e10})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    return result && obj.value > 1.4e10 && obj.value < 1.6e10;
}());

// Test: Negative scientific notation
static_assert([]() constexpr {
    struct Simple { double value; };

    std::string json = R"({"value": -2.5e-3})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    return result && obj.value > -0.003 && obj.value < -0.002;
}());

// Test: Array of floats with zeros
static_assert([]() constexpr {
    struct Config { std::vector<double> values; };

    std::string json = R"({"values": [0.0, 0.5, 1.0, 0.25]})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Config obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    return result
        && obj.values.size() == 4
        && obj.values[0] == 0.0
        && obj.values[1] > 0.4 && obj.values[1] < 0.6
        && obj.values[2] == 1.0
        && obj.values[3] > 0.24 && obj.values[3] < 0.26;
}());

// ============================================================================
// Tests for RFC 8259 Leading Zero Rejection
// ============================================================================

// Test: Leading zero should be rejected (01 is invalid JSON)
static_assert([]() constexpr {
    struct Simple { int value; };

    std::string json = R"({"value": 01})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    // Should fail - leading zeros not allowed
    return !result && result.readerError() == JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER;
}());

// Test: Leading zeros in decimal (007 is invalid)
static_assert([]() constexpr {
    struct Simple { int value; };

    std::string json = R"({"value": 007})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    // Should fail
    return !result && result.readerError() == JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER;
}());

// Test: -01 is also invalid
static_assert([]() constexpr {
    struct Simple { int value; };

    std::string json = R"({"value": -01})";
    std::size_t pos = 0;

    SinglePassIteratorWithEnd<std::string> begin(json, &pos, json.size());
    SinglePassIteratorWithEnd<std::string> end_it;

    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end_it);

    // Should fail
    return !result && result.readerError() == JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER;
}());

