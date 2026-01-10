#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/static_schema.hpp>
#include <JsonFusion/cbor.hpp>
#include <array>
#include <utility>

using namespace TestHelpers;
using namespace JsonFusion;
using namespace JsonFusion::static_schema;

// ============================================================================
// Test: String Streaming - Consuming and Producing String Streamers
// ============================================================================

// ============================================================================
// Part 1: Consuming String Streamer (for parsing)
// ============================================================================

// Simple string consumer that accumulates characters
struct StringConsumer {
    std::array<char, 128> buffer{};
    std::size_t length = 0;
    bool finalized = false;
    
    constexpr bool consume(const char* data, std::size_t len) {
        if (length + len > buffer.size()) return false;
        for (std::size_t i = 0; i < len; ++i) {
            buffer[length++] = data[i];
        }
        return true;
    }
    
    constexpr bool finalize(bool success) {
        finalized = success;
        return success;
    }
    
    constexpr void reset() {
        length = 0;
        finalized = false;
    }
    
    // Helper to get string_view of accumulated data
    constexpr std::string_view view() const {
        return std::string_view(buffer.data(), length);
    }
};

// Verify concept satisfaction
static_assert(ConsumingStringStreamerLike<StringConsumer>);
static_assert(ParsableStringLike<StringConsumer>);

// Test 1: Basic string consumer parsing
static_assert(
    []() constexpr {
        StringConsumer consumer{};
        std::string json = R"("hello world")";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.view() == "hello world"
            && consumer.finalized == true;
    }(),
    "Basic string consumer parsing"
);

// Test 2: Empty string
static_assert(
    []() constexpr {
        StringConsumer consumer{};
        std::string json = R"("")";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.length == 0 && consumer.finalized == true;
    }(),
    "Empty string parsing"
);

// Test 3: String with escapes
static_assert(
    []() constexpr {
        StringConsumer consumer{};
        std::string json = R"("hello\nworld")";
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        // Should have newline character
        return consumer.length == 11 
            && consumer.buffer[5] == '\n';
    }(),
    "String with escape sequences"
);

// Test 4: Unicode escape
static_assert(
    []() constexpr {
        StringConsumer consumer{};
        std::string json = R"("A\u0042C")";  // \u0042 = 'B'
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        return consumer.view() == "ABC";
    }(),
    "String with unicode escape"
);

// Test 5: Consumer as struct field
struct WithStringConsumer {
    StringConsumer name;
    int value;
};

static_assert(
    []() constexpr {
        WithStringConsumer obj{};
        std::string json = R"({"name": "test-name", "value": 42})";
        auto result = JsonFusion::Parse(obj, json);
        if (!result) return false;
        return obj.name.view() == "test-name"
            && obj.value == 42;
    }(),
    "String consumer as struct field"
);

// Test 6: Large string to test chunking - verify multiple chunks actually used
struct ChunkCountingConsumer {
    std::array<char, 128> buffer{};
    std::size_t length = 0;
    mutable std::size_t consume_count = 0;  // Track number of consume() calls
    
    constexpr bool consume(const char* data, std::size_t len) {
        consume_count++;
        if (length + len > buffer.size()) return false;
        for (std::size_t i = 0; i < len; ++i) {
            buffer[length++] = data[i];
        }
        return true;
    }
    
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() { 
        length = 0; 
        consume_count = 0;
    }
    
    constexpr std::string_view view() const {
        return std::string_view(buffer.data(), length);
    }
};

static_assert(ConsumingStringStreamerLike<ChunkCountingConsumer>);

static_assert(
    []() constexpr {
        ChunkCountingConsumer consumer{};
        // String longer than default chunk size (64 bytes) - should require at least 2 chunks
        std::string json = R"("0123456789012345678901234567890123456789012345678901234567890123456789")";  // 70 chars
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        // Verify: length is correct AND multiple consume() calls happened
        return consumer.length == 70 
            && consumer.consume_count >= 2;  // At least 2 chunks used!
    }(),
    "Large string parsed in multiple chunks (verified)"
);

// Test 7: Consumer finalize called correctly
struct FinalizeTracker {
    std::array<char, 64> buffer{};
    std::size_t length = 0;
    bool finalize_called = false;
    bool finalize_success = false;
    
    constexpr bool consume(const char* data, std::size_t len) {
        if (length + len > buffer.size()) return false;
        for (std::size_t i = 0; i < len; ++i) {
            buffer[length++] = data[i];
        }
        return true;
    }
    
    constexpr bool finalize(bool success) { 
        finalize_called = true;
        finalize_success = success;
        return success; 
    }
    
    constexpr void reset() { 
        length = 0; 
        finalize_called = false;
        finalize_success = false;
    }
};

static_assert(ConsumingStringStreamerLike<FinalizeTracker>);

static_assert(
    []() constexpr {
        FinalizeTracker consumer{};
        std::string json = R"("test")";
        auto result = JsonFusion::Parse(consumer, json);
        return result 
            && consumer.finalize_called 
            && consumer.finalize_success
            && consumer.length == 4;
    }(),
    "Consumer finalize called correctly"
);

// Test 8: Consumer with context
struct ContextualConsumer {
    std::array<char, 128> buffer{};
    std::size_t length = 0;
    mutable int* char_count_ctx = nullptr;  // Context: count characters
    
    struct Context {
        int total_chars = 0;
    };
    
    constexpr void set_jsonfusion_context(Context* ctx) const {
        if (ctx) {
            char_count_ctx = &ctx->total_chars;
        }
    }
    
    constexpr bool consume(const char* data, std::size_t len) {
        if (length + len > buffer.size()) return false;
        for (std::size_t i = 0; i < len; ++i) {
            buffer[length++] = data[i];
            if (char_count_ctx) {
                (*char_count_ctx)++;
            }
        }
        return true;
    }
    
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() { length = 0; }
    
    constexpr std::string_view view() const {
        return std::string_view(buffer.data(), length);
    }
};

static_assert(ConsumingStringStreamerLike<ContextualConsumer>);

static_assert(
    []() constexpr {
        ContextualConsumer consumer{};
        ContextualConsumer::Context ctx{};
        std::string json = R"("hello")";
        auto result = JsonFusion::Parse(consumer, json, &ctx);
        if (!result) return false;
        return consumer.view() == "hello"
            && ctx.total_chars == 5;  // Context received and used
    }(),
    "String consumer with context"
);

// Test 9: Consumer with context in struct field
struct WithContextualConsumer {
    ContextualConsumer name;
    int value;
};

static_assert(
    []() constexpr {
        WithContextualConsumer obj{};
        ContextualConsumer::Context ctx{};
        std::string json = R"({"name": "test-name", "value": 42})";
        auto result = JsonFusion::Parse(obj, json, &ctx);
        if (!result) return false;
        return obj.name.view() == "test-name"
            && obj.value == 42
            && ctx.total_chars == 9;  // "test-name" has 9 chars
    }(),
    "String consumer with context in struct field"
);

// ============================================================================
// Part 2: Producing String Streamer (for serialization)
// ============================================================================

// Simple string producer from fixed data
struct StringProducer {
    const char* data_ = nullptr;
    std::size_t len_ = 0;
    mutable std::size_t pos_ = 0;
    
    constexpr StringProducer() = default;
    constexpr StringProducer(const char* d, std::size_t l) : data_(d), len_(l) {}
    
    constexpr std::pair<std::size_t, bool> read_chunk(char* buf, std::size_t capacity) const {
        if (!data_ || pos_ >= len_) {
            return {0, true};  // No more data
        }
        
        std::size_t remaining = len_ - pos_;
        std::size_t to_copy = remaining < capacity ? remaining : capacity;
        
        for (std::size_t i = 0; i < to_copy; ++i) {
            buf[i] = data_[pos_++];
        }
        
        bool done = (pos_ >= len_);
        return {to_copy, done};
    }
    
    constexpr std::size_t total_size() const {
        return len_;
    }
    
    constexpr void reset() const {
        pos_ = 0;
    }
};

// Verify concept satisfaction
static_assert(ProducingStringStreamerLike<StringProducer>);
static_assert(SerializableStringLike<StringProducer>);

// Test 8: Basic string producer serialization
static_assert(
    []() constexpr {
        const char data[] = "hello world";
        StringProducer producer{data, 11};
        std::string output;
        JsonFusion::Serialize(producer, output);
        return output == R"("hello world")";
    }(),
    "Basic string producer serialization"
);

// Test 9: Empty string producer
static_assert(
    []() constexpr {
        StringProducer producer{nullptr, 0};
        std::string output;
        JsonFusion::Serialize(producer, output);
        return output == R"("")";
    }(),
    "Empty string producer"
);

// Test 10: Producer as struct field
struct WithStringProducer {
    StringProducer name;
    int value;
};

static_assert(
    []() constexpr {
        const char name_data[] = "test";
        WithStringProducer obj{};
        obj.name = StringProducer{name_data, 4};
        obj.value = 42;
        std::string output;
        JsonFusion::Serialize(obj, output);
        // Should produce {"name":"test","value":42}
        return output.find("\"name\"") != std::string::npos
            && output.find("\"test\"") != std::string::npos
            && output.find("42") != std::string::npos;
    }(),
    "String producer as struct field"
);

// Test 11: Producer with special characters (escaping)
static_assert(
    []() constexpr {
        const char data[] = "line1\nline2";
        StringProducer producer{data, 11};
        std::string output;
        JsonFusion::Serialize(producer, output);
        // Should have escaped newline
        return output.find("\\n") != std::string::npos;
    }(),
    "String producer with escaping"
);

// Test 12: Producer with unknown size (streaming)
struct StreamingProducer {
    const char* data_ = nullptr;
    std::size_t len_ = 0;
    mutable std::size_t pos_ = 0;
    
    constexpr StreamingProducer() = default;
    constexpr StreamingProducer(const char* d, std::size_t l) : data_(d), len_(l) {}
    
    constexpr std::pair<std::size_t, bool> read_chunk(char* buf, std::size_t capacity) const {
        if (!data_ || pos_ >= len_) {
            return {0, true};
        }
        
        std::size_t remaining = len_ - pos_;
        std::size_t to_copy = remaining < capacity ? remaining : capacity;
        
        for (std::size_t i = 0; i < to_copy; ++i) {
            buf[i] = data_[pos_++];
        }
        
        return {to_copy, pos_ >= len_};
    }
    
    // Returns SIZE_MAX to indicate unknown size (streaming)
    constexpr std::size_t total_size() const {
        return std::numeric_limits<std::size_t>::max();
    }
    
    constexpr void reset() const {
        pos_ = 0;
    }
};

static_assert(ProducingStringStreamerLike<StreamingProducer>);
static_assert(SerializableStringLike<StreamingProducer>);

static_assert(
    []() constexpr {
        const char data[] = "streaming";
        StreamingProducer producer{data, 9};
        std::string output;
        JsonFusion::Serialize(producer, output);
        return output == R"("streaming")";
    }(),
    "Streaming producer (unknown size)"
);

// Test 13: Producer with context
struct ContextualProducer {
    mutable const char* data_ = nullptr;
    mutable std::size_t len_ = 0;
    mutable std::size_t pos_ = 0;
    
    struct Context {
        const char* data;
        std::size_t len;
    };
    
    constexpr void set_jsonfusion_context(Context* ctx) const {
        if (ctx) {
            data_ = ctx->data;
            len_ = ctx->len;
        }
    }
    
    constexpr std::pair<std::size_t, bool> read_chunk(char* buf, std::size_t capacity) const {
        if (!data_ || pos_ >= len_) {
            return {0, true};
        }
        
        std::size_t remaining = len_ - pos_;
        std::size_t to_copy = remaining < capacity ? remaining : capacity;
        
        for (std::size_t i = 0; i < to_copy; ++i) {
            buf[i] = data_[pos_++];
        }
        
        return {to_copy, pos_ >= len_};
    }
    
    constexpr std::size_t total_size() const {
        return len_;
    }
    
    constexpr void reset() const {
        pos_ = 0;
    }
};

static_assert(ProducingStringStreamerLike<ContextualProducer>);

static_assert(
    []() constexpr {
        const char data[] = "from-context";
        ContextualProducer producer{};
        ContextualProducer::Context ctx{data, 12};
        std::string output;
        JsonFusion::Serialize(producer, output, &ctx);
        return output == R"("from-context")";
    }(),
    "String producer with context"
);

// Test 16: Producer with context in struct field
struct WithContextualProducer {
    ContextualProducer name;
    int value;
};

static_assert(
    []() constexpr {
        const char data[] = "ctx-name";
        WithContextualProducer obj{};
        obj.value = 99;
        ContextualProducer::Context ctx{data, 8};
        std::string output;
        JsonFusion::Serialize(obj, output, &ctx);
        return output.find("\"ctx-name\"") != std::string::npos
            && output.find("99") != std::string::npos;
    }(),
    "String producer with context in struct field"
);

// Test 17: Multiple producers in struct
struct MultipleStringProducers {
    StringProducer first;
    StringProducer second;
};

static_assert(
    []() constexpr {
        const char data1[] = "one";
        const char data2[] = "two";
        MultipleStringProducers obj{};
        obj.first = StringProducer{data1, 3};
        obj.second = StringProducer{data2, 3};
        std::string output;
        JsonFusion::Serialize(obj, output);
        return output.find("\"one\"") != std::string::npos
            && output.find("\"two\"") != std::string::npos;
    }(),
    "Multiple string producers in struct"
);

// Test 14: Verify producer uses multiple chunks for large string
struct ChunkCountingProducer {
    const char* data_ = nullptr;
    std::size_t len_ = 0;
    mutable std::size_t pos_ = 0;
    mutable std::size_t read_chunk_count = 0;  // Track number of read_chunk() calls
    
    constexpr ChunkCountingProducer() = default;
    constexpr ChunkCountingProducer(const char* d, std::size_t l) : data_(d), len_(l) {}
    
    constexpr std::pair<std::size_t, bool> read_chunk(char* buf, std::size_t capacity) const {
        read_chunk_count++;
        
        if (!data_ || pos_ >= len_) {
            return {0, true};
        }
        
        std::size_t remaining = len_ - pos_;
        std::size_t to_copy = remaining < capacity ? remaining : capacity;
        
        for (std::size_t i = 0; i < to_copy; ++i) {
            buf[i] = data_[pos_++];
        }
        
        bool done = (pos_ >= len_);
        return {to_copy, done};
    }
    
    constexpr std::size_t total_size() const { return len_; }
    
    constexpr void reset() const {
        pos_ = 0;
        read_chunk_count = 0;
    }
};

static_assert(ProducingStringStreamerLike<ChunkCountingProducer>);

static_assert(
    []() constexpr {
        // String longer than default cursor buffer (64 bytes) - should require multiple read_chunk() calls
        const char data[] = "This is a very long string that exceeds the default buffer size of 64 bytes and should trigger multiple read_chunk calls!";
        ChunkCountingProducer producer{data, 122};  // 122 chars
        std::string output;
        JsonFusion::Serialize(producer, output);
        // Verify: output contains the string AND multiple read_chunk() calls happened
        return output.find("This is a very long string") != std::string::npos
            && producer.read_chunk_count >= 2;  // At least 2 chunks read!
    }(),
    "Large string serialized in multiple chunks (verified)"
);

// Test 15: Reset works correctly
static_assert(
    []() constexpr {
        const char data[] = "test";
        StringProducer producer{data, 4};
        
        std::string output1;
        JsonFusion::Serialize(producer, output1);
        
        producer.reset();
        
        std::string output2;
        JsonFusion::Serialize(producer, output2);
        
        return output1 == output2;
    }(),
    "Producer reset works correctly"
);

// ============================================================================
// Part 3: Custom buffer_size in streamers
// ============================================================================

// Consumer with custom buffer size
struct SmallBufferConsumer {
    static constexpr std::size_t buffer_size = 8;  // Small buffer
    
    std::array<char, 128> buffer{};
    std::size_t length = 0;
    mutable std::size_t consume_count = 0;  // Track chunk count
    
    constexpr bool consume(const char* data, std::size_t len) {
        consume_count++;
        // Verify chunks are <= buffer_size
        if (len > buffer_size) return false;
        if (length + len > buffer.size()) return false;
        for (std::size_t i = 0; i < len; ++i) {
            buffer[length++] = data[i];
        }
        return true;
    }
    
    constexpr bool finalize(bool success) { return success; }
    constexpr void reset() { 
        length = 0; 
        consume_count = 0;
    }
    
    constexpr std::string_view view() const {
        return std::string_view(buffer.data(), length);
    }
};

static_assert(ConsumingStringStreamerLike<SmallBufferConsumer>);

// Verify buffer_size is picked up from streamer
static_assert(static_schema::string_write_cursor<SmallBufferConsumer>::buffer_size() == 8);

static_assert(
    []() constexpr {
        SmallBufferConsumer consumer{};
        std::string json = R"("hello world")";  // 11 chars with buffer_size=8 -> requires 2+ chunks
        auto result = JsonFusion::Parse(consumer, json);
        if (!result) return false;
        // Verify: correct content AND multiple chunks used
        return consumer.view() == "hello world"
            && consumer.consume_count >= 2;  // At least 2 chunks for 11 chars with 8-byte buffer!
    }(),
    "Consumer with custom buffer_size uses multiple chunks"
);

// Producer with small buffer size - to verify multiple chunks
struct SmallBufferProducer {
    static constexpr std::size_t buffer_size = 10;  // Small buffer
    
    const char* data_ = nullptr;
    std::size_t len_ = 0;
    mutable std::size_t pos_ = 0;
    mutable std::size_t read_chunk_count = 0;
    
    constexpr SmallBufferProducer() = default;
    constexpr SmallBufferProducer(const char* d, std::size_t l) : data_(d), len_(l) {}
    
    constexpr std::pair<std::size_t, bool> read_chunk(char* buf, std::size_t capacity) const {
        read_chunk_count++;
        
        if (!data_ || pos_ >= len_) {
            return {0, true};
        }
        
        std::size_t remaining = len_ - pos_;
        std::size_t to_copy = remaining < capacity ? remaining : capacity;
        
        for (std::size_t i = 0; i < to_copy; ++i) {
            buf[i] = data_[pos_++];
        }
        
        return {to_copy, pos_ >= len_};
    }
    
    constexpr std::size_t total_size() const { return len_; }
    constexpr void reset() const { 
        pos_ = 0; 
        read_chunk_count = 0;
    }
};

static_assert(ProducingStringStreamerLike<SmallBufferProducer>);

// Verify buffer_size is picked up from streamer
static_assert(static_schema::string_read_cursor<SmallBufferProducer>::buffer_size == 10);

static_assert(
    []() constexpr {
        const char data[] = "This needs multiple chunks!";  // 27 chars with buffer_size=10
        SmallBufferProducer producer{data, 27};
        std::string output;
        JsonFusion::Serialize(producer, output);
        // Verify: correct output AND multiple read_chunk() calls
        return output == R"("This needs multiple chunks!")"
            && producer.read_chunk_count >= 3;  // At least 3 chunks for 27 chars with 10-byte buffer!
    }(),
    "Producer with small buffer_size uses multiple chunks"
);

// Producer with custom buffer size  
struct LargeBufferProducer {
    static constexpr std::size_t buffer_size = 256;  // Large buffer
    
    const char* data_ = nullptr;
    std::size_t len_ = 0;
    mutable std::size_t pos_ = 0;
    
    constexpr LargeBufferProducer() = default;
    constexpr LargeBufferProducer(const char* d, std::size_t l) : data_(d), len_(l) {}
    
    constexpr std::pair<std::size_t, bool> read_chunk(char* buf, std::size_t capacity) const {
        if (!data_ || pos_ >= len_) {
            return {0, true};
        }
        
        std::size_t remaining = len_ - pos_;
        std::size_t to_copy = remaining < capacity ? remaining : capacity;
        
        for (std::size_t i = 0; i < to_copy; ++i) {
            buf[i] = data_[pos_++];
        }
        
        return {to_copy, pos_ >= len_};
    }
    
    constexpr std::size_t total_size() const { return len_; }
    constexpr void reset() const { pos_ = 0; }
};

static_assert(ProducingStringStreamerLike<LargeBufferProducer>);

// Verify buffer_size is picked up from streamer
static_assert(static_schema::string_read_cursor<LargeBufferProducer>::buffer_size == 256);

static_assert(
    []() constexpr {
        const char data[] = "large buffer test";
        LargeBufferProducer producer{data, 17};
        std::string output;
        JsonFusion::Serialize(producer, output);
        return output == R"("large buffer test")";
    }(),
    "Producer with custom buffer_size"
);

// ============================================================================
// Part 4: CBOR String Chunking Tests
// ============================================================================

// Test that CBOR writer's chunked string interface works correctly
// CBOR has two string encoding modes:
// 1. Definite-length: when total_size() is known (not SIZE_MAX)
// 2. Indefinite-length: when total_size() is SIZE_MAX (0x7F...chunks...0xFF)

// Test 1: CBOR with known-size producer (definite-length encoding)
static_assert(
    []() constexpr {
        const char data[] = "cbor test";
        StringProducer producer{data, 9};
        
        std::array<char, 128> buffer{};
        auto it = buffer.begin();
        auto res = SerializeWithWriter(producer, CborWriter(it, buffer.end()));
        if (!res) return false;
        
        // Parse it back to verify
        std::string result;
        if (!ParseWithReader(result, CborReader(buffer.begin(), buffer.begin() + res.bytesWritten()))) {
            return false;
        }
        
        return result == "cbor test";
    }(),
    "CBOR: String producer with known size (definite-length)"
);

// Test 2: CBOR with unknown-size producer (indefinite-length encoding)
static_assert(
    []() constexpr {
        const char data[] = "indefinite";
        StreamingProducer producer{data, 10};  // total_size() returns SIZE_MAX
        
        std::array<char, 128> buffer{};
        auto it = buffer.begin();
        auto res = SerializeWithWriter(producer, CborWriter(it, buffer.end()));
        if (!res) return false;
        
        // Verify indefinite-length encoding: should start with 0x7F (indefinite text string)
        unsigned char first_byte = static_cast<unsigned char>(buffer[0]);
        if (first_byte != 0x7F) return false;
        
        // Verify it ends with 0xFF (break marker)
        unsigned char last_byte = static_cast<unsigned char>(buffer[res.bytesWritten() - 1]);
        if (last_byte != 0xFF) return false;
        
        return true;  // Successfully serialized with indefinite-length encoding
    }(),
    "CBOR: String producer with unknown size (indefinite-length encoding verified)"
);

// Test 3: CBOR with multi-chunk string (verify chunking works)
static_assert(
    []() constexpr {
        // Use SmallBufferProducer to force multiple chunks
        const char data[] = "This string is long enough to require multiple chunks!";
        SmallBufferProducer producer{data, 54};  // buffer_size=10, so requires 6 chunks
        
        std::array<char, 128> buffer{};
        auto it = buffer.begin();
        auto res = SerializeWithWriter(producer, CborWriter(it, buffer.end()));
        if (!res) return false;
        
        // Verify multiple chunks were read
        if (producer.read_chunk_count < 6) return false;
        
        // Parse it back to verify correctness
        std::string result;
        if (!ParseWithReader(result, CborReader(buffer.begin(), buffer.begin() + res.bytesWritten()))) {
            return false;
        }
        
        return result == "This string is long enough to require multiple chunks!";
    }(),
    "CBOR: Multi-chunk string producer (verified chunking)"
);

// Test 4: CBOR string consumer parsing
static_assert(
    []() constexpr {
        ChunkCountingConsumer consumer{};
        
        // Create CBOR-encoded string manually for testing
        std::array<char, 128> cbor_data{};
        auto it = cbor_data.begin();
        std::string input = "cbor consumer test";
        auto res = SerializeWithWriter(input, CborWriter(it, cbor_data.end()));
        if (!res) return false;
        
        // Parse into consumer
        if (!ParseWithReader(consumer, CborReader(cbor_data.begin(), cbor_data.begin() + res.bytesWritten()))) {
            return false;
        }
        
        return consumer.view() == "cbor consumer test";
    }(),
    "CBOR: String consumer parsing"
);

// Test 5: CBOR roundtrip with streaming producer and consumer
static_assert(
    []() constexpr {
        const char data[] = "roundtrip test string";
        StringProducer producer{data, 21};
        
        // Serialize with CBOR
        std::array<char, 128> buffer{};
        auto it = buffer.begin();
        auto res = SerializeWithWriter(producer, CborWriter(it, buffer.end()));
        if (!res) return false;
        
        // Parse back with consumer
        StringConsumer consumer{};
        if (!ParseWithReader(consumer, CborReader(buffer.begin(), buffer.begin() + res.bytesWritten()))) {
            return false;
        }
        
        return consumer.view() == "roundtrip test string";
    }(),
    "CBOR: Roundtrip with producer and consumer"
);

