#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <string_view>
#include <array>
#include <optional>

// ============================================================================
// Custom Input Iterator - Blocks Range Operations
// ============================================================================

/// A true forward iterator that can ONLY advance one byte at a time.
/// This proves the parser doesn't require pointer arithmetic or random access.
template<typename Container>
class ByteByByteInputIterator {
    const Container* container;
    std::size_t index;

public:
    // Iterator traits
    using iterator_category = std::input_iterator_tag;
    using value_type = char;
    using difference_type = std::ptrdiff_t;

    constexpr ByteByByteInputIterator() : container(nullptr), index(0) {}
    
    constexpr ByteByByteInputIterator(const Container& c, std::size_t idx = 0)
        : container(&c), index(idx) {}

    // Only forward iteration allowed
    constexpr ByteByByteInputIterator& operator++() {
        ++index;
        return *this;
    }

    constexpr ByteByByteInputIterator operator++(int) {
        auto temp = *this;
        ++index;
        return temp;
    }

    constexpr char operator*() const {
        return (*container)[index];
    }

    constexpr bool operator==(const ByteByByteInputIterator& other) const {
        return container == other.container && index == other.index;
    }

    // Explicitly DELETE range operations to prove they're not used
    constexpr ByteByByteInputIterator operator+(difference_type) const = delete;
    constexpr ByteByByteInputIterator operator-(difference_type) const = delete;
    constexpr difference_type operator-(const ByteByByteInputIterator&) const = delete;
    constexpr ByteByByteInputIterator& operator+=(difference_type) = delete;
    constexpr ByteByByteInputIterator& operator-=(difference_type) = delete;
    constexpr char operator[](difference_type) const = delete;
    constexpr bool operator<(const ByteByByteInputIterator&) const = delete;
    constexpr bool operator>(const ByteByByteInputIterator&) const = delete;
    constexpr bool operator<=(const ByteByByteInputIterator&) const = delete;
    constexpr bool operator>=(const ByteByByteInputIterator&) const = delete;
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
    
    ByteByByteInputIterator<std::string> begin(json, 0);
    ByteByByteInputIterator<std::string> end(json, json.size());
    
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
    
    ByteByByteInputIterator<std::string> begin(json, 0);
    ByteByByteInputIterator<std::string> end(json, json.size());
    
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
    
    ByteByByteInputIterator<std::string> begin(json, 0);
    ByteByByteInputIterator<std::string> end(json, json.size());
    
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
    
    ByteByByteInputIterator<std::string> begin(json, 0);
    ByteByByteInputIterator<std::string> end(json, json.size());
    
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
    
    ByteByByteInputIterator<std::string> begin(json, 0);
    ByteByByteInputIterator<std::string> end(json, json.size());
    
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
    
    ByteByByteInputIterator<std::string> begin(json, 0);
    ByteByByteInputIterator<std::string> end(json, json.size());
    
    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end);
    
    // Should fail with parse error
    return !result && result.readerError() == JsonFusion::JsonIteratorReaderError::ILLFORMED_NUMBER;
}());

// Test: Whitespace handling with byte-by-byte parsing
static_assert([]() constexpr {
    struct Simple { int value; };

    std::string json = R"(  {  "value"  :  123  }  )";
    
    ByteByByteInputIterator<std::string> begin(json, 0);
    ByteByByteInputIterator<std::string> end(json, json.size());
    
    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end);
    
    return result && obj.value == 123;
}());

// Test: String escapes with byte-by-byte parsing
static_assert([]() constexpr {
    struct Simple { std::string text; };

    std::string json = R"({"text": "hello\nworld\t\u0041"})";
    
    ByteByByteInputIterator<std::string> begin(json, 0);
    ByteByByteInputIterator<std::string> end(json, json.size());
    
    Simple obj{};
    auto result = JsonFusion::Parse(obj, begin, end);
    
    return result 
        && obj.text.size() == 13  // "hello\nworld\tA"
        && obj.text[5] == '\n'
        && obj.text[11] == '\t'
        && obj.text[12] == 'A';
}());

