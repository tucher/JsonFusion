#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <limits>
#include <cmath>
#include <type_traits>

#include "reader_concept.hpp"
#include "writer_concept.hpp"
namespace JsonFusion {

template<class It, class Sent>
class CborReader {
public:
    enum class ParseError {
        NO_ERROR,
        UNEXPECTED_END_OF_DATA,
        EXCESS_CHARACTERS,
        NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE,
        SKIPPING_STACK_OVERFLOW,
        NOT_IMPLEMENTED,
        ILLFORMED_NUMBER,
        ILLFORMED_VALUE,
        SKIP_ERROR
    };
    using error_type = ParseError;

    using iterator_type = It;

    struct ArrayFrame {
        std::uint64_t remaining = 0;  // elements left in this array (unused if indefinite)
        bool indefinite = false;      // true if indefinite-length array
    };

    struct MapFrame {
        std::uint64_t remaining_pairs = 0; // key/value pairs left (unused if indefinite)
        bool indefinite = false;           // true if indefinite-length map
    };

    CborReader(It & first, Sent last) noexcept
        : m_errorPos(first), cur_(first), end_(last)
        , err_(ParseError::NO_ERROR)
    {}

    // ========== Introspection ==========

    iterator_type current() const noexcept {
        return cur_;
    }

    ParseError getError() const noexcept {
        return err_;
    }

    // ========== Primitive Value Parsing ==========

    // CBOR has no whitespace; this just checks for null (simple value 22).
    constexpr reader::TryParseStatus start_value_and_try_read_null() {
        if (!ensure_bytes()) {
            return reader::TryParseStatus::error;
        }

        std::uint8_t ib = *cur_;
        std::uint8_t major = ib >> 5;
        std::uint8_t ai    = ib & 0x1F;

        if (major == 7 && ai == 22) { // null
            ++cur_;
            return reader::TryParseStatus::ok;
        }

        return reader::TryParseStatus::no_match;
    }

    constexpr reader::TryParseStatus read_bool(bool& b) {
        if (!ensure_bytes()) {
            return reader::TryParseStatus::error;
        }

        std::uint8_t ib = *cur_;
        std::uint8_t major = ib >> 5;
        std::uint8_t ai    = ib & 0x1F;

        if (major != 7) {
            return reader::TryParseStatus::no_match;
        }

        if (ai == 20) {        // false
            b = false;
            ++cur_;
            return reader::TryParseStatus::ok;
        } else if (ai == 21) { // true
            b = true;
            ++cur_;
            return reader::TryParseStatus::ok;
        }

        return reader::TryParseStatus::no_match;
    }

    template<class NumberT>
    constexpr reader::TryParseStatus read_number(NumberT& storage) {
        static_assert(std::is_integral_v<NumberT> || std::is_floating_point_v<NumberT>,
                      "CborReader::read_number requires integral or floating type");

        if (!ensure_bytes()) {
            return reader::TryParseStatus::error;
        }

        const std::uint8_t ib = *cur_;
        const std::uint8_t major = ib >> 5;
        const std::uint8_t ai    = ib & 0x1F;

        // ----- Integral storage -----
        if constexpr (std::is_integral_v<NumberT>) {
            if (major == 0) { // unsigned
                std::uint64_t uval;
                if (!decode_uint(ai, uval)) {
                    return reader::TryParseStatus::error;
                }

                if (uval > static_cast<std::uint64_t>(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return reader::TryParseStatus::error;
                }
                storage = static_cast<NumberT>(uval);
                return reader::TryParseStatus::ok;
            } else if (major == 1) { // negative: value = -1 - n
                std::uint64_t n;
                if (!decode_uint(ai, n)) {
                    return reader::TryParseStatus::error;
                }
                std::int64_t sval = -static_cast<std::int64_t>(n) - 1;

                if constexpr (std::is_unsigned_v<NumberT>) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return reader::TryParseStatus::error;
                } else {
                    if (sval < std::numeric_limits<NumberT>::lowest() ||
                        sval > std::numeric_limits<NumberT>::max()) {
                        setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                        return reader::TryParseStatus::error;
                    }
                    storage = static_cast<NumberT>(sval);
                    return reader::TryParseStatus::ok;
                }
            }

            // allow float encodings -> integer if they fit?
            if (major == 7 && (ai == 25 || ai == 26 || ai == 27)) {
                double dv{};
                if (!decode_float(ai, dv)) {
                    return reader::TryParseStatus::error;
                }
                if (dv < static_cast<double>(std::numeric_limits<NumberT>::lowest()) ||
                    dv > static_cast<double>(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return reader::TryParseStatus::error;
                }
                storage = static_cast<NumberT>(dv);
                return reader::TryParseStatus::ok;
            }

            return reader::TryParseStatus::no_match;
        }

        // ----- Floating storage -----
        if constexpr (std::is_floating_point_v<NumberT>) {
            if (major == 7 && (ai == 25 || ai == 26 || ai == 27)) {
                double dv{};
                if (!decode_float(ai, dv)) {
                    return reader::TryParseStatus::error;
                }
                if (dv < static_cast<double>(std::numeric_limits<NumberT>::lowest()) ||
                    dv > static_cast<double>(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return reader::TryParseStatus::error;
                }
                storage = static_cast<NumberT>(dv);
                return reader::TryParseStatus::ok;
            }

            // int → float is allowed
            if (major == 0) {
                std::uint64_t uval;
                if (!decode_uint(ai, uval)) {
                    return reader::TryParseStatus::error;
                }
                double dv = static_cast<double>(uval);
                if (dv < static_cast<double>(std::numeric_limits<NumberT>::lowest()) ||
                    dv > static_cast<double>(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return reader::TryParseStatus::error;
                }
                storage = static_cast<NumberT>(dv);
                return reader::TryParseStatus::ok;
            } else if (major == 1) {
                std::uint64_t n;
                if (!decode_uint(ai, n)) {
                    return reader::TryParseStatus::error;
                }
                double dv = -static_cast<double>(n) - 1.0;
                if (dv < static_cast<double>(std::numeric_limits<NumberT>::lowest()) ||
                    dv > static_cast<double>(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return reader::TryParseStatus::error;
                }
                storage = static_cast<NumberT>(dv);
                return reader::TryParseStatus::ok;
            }

            return reader::TryParseStatus::no_match;
        }
    }

    // ========== String parsing (chunked) ==========

    // Parser calls this for:
    //   - JSON-like "string values",
    //   - JSON-like "object keys" (for struct field lookup / maps).
    constexpr reader::StringChunkResult read_string_chunk(char* out,
                                                               std::size_t capacity) {
        reader::StringChunkResult res{};
        res.status        = reader::StringChunkStatus::error;
        res.bytes_written = 0;
        res.done          = false;

        if (capacity == 0) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return res;
        }

        if (!value_str_active_) {
            // First call for this string.
            if (!ensure_bytes()) {
                return res;
            }

            const std::uint8_t ib = *cur_;
            const std::uint8_t major = ib >> 5;
            const std::uint8_t ai    = ib & 0x1F;

            // Treat both text (major 3) and byte string (major 2) as "string-like".
            if (major != 2 && major != 3) {
                res.status = reader::StringChunkStatus::no_match;
                return res;
            }

            std::uint64_t len;
            if (!decode_length(ai, len)) {
                // decode_length sets error on failure.
                return res;
            }

            // After decode_length, cur_ points to the first byte of string data.
            // For forward-only iterators, we don't save this position - just track progress.
            value_str_len_    = static_cast<std::size_t>(len);
            value_str_offset_ = 0;
            value_str_active_ = true;
        }

        const std::size_t remaining = value_str_len_ - value_str_offset_;
        const std::size_t n         = remaining < capacity ? remaining : capacity;
        
        for(std::size_t i = 0; i < n; i ++) {
            if (!ensure_bytes()) {  // Check before each read
                return res;  // Error already set by ensure_bytes()
            }
            out[i] = *cur_;
            ++cur_;
        }
        value_str_offset_ += n;

        res.status        = reader::StringChunkStatus::ok;
        res.bytes_written = n;
        res.done          = (value_str_offset_ >= value_str_len_);

        if (res.done) {
            // Advance underlying cursor past the string bytes.
            reset_value_string_state();
        }

        return res;
    }

    // Index-based keys (for indexes_as_keys).
    constexpr bool read_key_as_index(std::size_t& out) {
        if (!ensure_bytes()) {
            return false;
        }

        const std::uint8_t ib = *cur_;
        const std::uint8_t major = ib >> 5;
        const std::uint8_t ai    = ib & 0x1F;

        // For CBOR, we only allow unsigned integer keys as indices.
        if (major == 0) {
            std::uint64_t uval;
            if (!decode_uint(ai, uval)) {
                return false; // error already set
            }

            if (uval > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
                setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                return false;
            }
            out = static_cast<std::size_t>(uval);
            return true;
        }

        // Negative or non-integer keys are invalid as indices.
        setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
        return false;
    }

    // ========== Arrays ==========

    constexpr reader::IterationStatus read_array_begin(ArrayFrame& frame) {
        reader::IterationStatus ret;
        reset_value_string_state();

        if (!ensure_bytes()) {
            ret.status = reader::TryParseStatus::error;
            return ret;
        }

        const std::uint8_t ib = *cur_;
        const std::uint8_t major = ib >> 5;
        const std::uint8_t ai    = ib & 0x1F;

        if (major != 4) {
            ret.status = reader::TryParseStatus::no_match;
            return ret;
        }

        // Check for indefinite-length array (ai == 31)
        if (ai == 31) {
            ++cur_;  // Consume the indefinite-length marker
            frame.indefinite = true;
            frame.remaining = 0;  // Unused for indefinite
            ret.has_value = true;  // Assume at least one element (will check for break)
            ret.status = reader::TryParseStatus::ok;
            return ret;
        }

        // Definite-length array
        std::uint64_t len;
        if (!decode_length(ai, len)) {
            ret.status = reader::TryParseStatus::error;
            return ret;
        }

        frame.indefinite = false;
        frame.remaining = len;
        ret.has_value = len != 0;
        ret.status = reader::TryParseStatus::ok;
        // cur_ now points to the first element (if len > 0) or to the first byte after
        // the array data (if len == 0 and no elements will be read).
        return ret;
    }

    // After each element:
    //   - element has been fully parsed (cur_ is at next element or after array)
    //   - we just decrement remaining, no cursor work needed.
    constexpr reader::IterationStatus advance_after_value(ArrayFrame& frame) {
        reset_value_string_state();

        reader::IterationStatus ret;
        ret.status = reader::TryParseStatus::ok;

        if (frame.indefinite) {
            // Check for break marker (0xFF)
            if (!ensure_bytes()) {
                ret.status = reader::TryParseStatus::error;
                return ret;
            }

            if (*cur_ == 0xFF) {  // Break marker (major 7, ai 31)
                ++cur_;  // Consume break
                ret.has_value = false;
                return ret;
            }

            // More elements available
            ret.has_value = true;
            return ret;
        }

        // Definite-length: decrement counter
        --frame.remaining;
        if (frame.remaining == 0) {
            ret.has_value = false;
            return ret;
        } else
            ret.has_value = true;


        return ret;
    }

    // ========== Objects (maps) ==========

    constexpr reader::IterationStatus read_map_begin(MapFrame& frame) {
        reader::IterationStatus ret;
        reset_value_string_state();

        if (!ensure_bytes()) {
            ret.status = reader::TryParseStatus::error;
            return ret;
        }

        const std::uint8_t ib = *cur_;
        const std::uint8_t major = ib >> 5;
        const std::uint8_t ai    = ib & 0x1F;

        if (major != 5) {
            ret.status = reader::TryParseStatus::no_match;
            return ret;
        }

        // Check for indefinite-length map (ai == 31)
        if (ai == 31) {
            ++cur_;  // Consume the indefinite-length marker
            frame.indefinite = true;
            frame.remaining_pairs = 0;  // Unused for indefinite
            ret.has_value = true;  // Assume at least one pair (will check for break)
            ret.status = reader::TryParseStatus::ok;
            return ret;
        }

        // Definite-length map
        std::uint64_t len;
        if (!decode_length(ai, len)) {
            ret.status = reader::TryParseStatus::error;
            return ret;
        }

        frame.indefinite = false;
        frame.remaining_pairs = len;
        ret.has_value = len > 0;
        ret.status = reader::TryParseStatus::ok;
        return ret;
    }


    // After key is fully read (via read_string_chunk or read_key_as_index)
    // parser calls consume_kv_separator() to switch to the value.
    // In CBOR, we already advanced cur_ past the key, so it's now at the value.
    constexpr bool move_to_value(MapFrame& frame) {
        (void)frame;
        reset_value_string_state();
        // Nothing to do: cur_ already at value.
        return true;
    }

    // After value is parsed, parser calls this to advance to next key.
    constexpr reader::IterationStatus advance_after_value(MapFrame& frame) {
        reader::IterationStatus ret;
        reset_value_string_state();
        ret.status = reader::TryParseStatus::ok;

        if (frame.indefinite) {
            // Check for break marker (0xFF)
            if (!ensure_bytes()) {
                ret.status = reader::TryParseStatus::error;
                return ret;
            }

            if (*cur_ == 0xFF) {  // Break marker (major 7, ai 31)
                ++cur_;  // Consume break
                ret.has_value = false;
                return ret;
            }

            // More pairs available
            ret.has_value = true;
            return ret;
        }

        // Definite-length: decrement counter
        --frame.remaining_pairs;
        if (frame.remaining_pairs == 0) {
            ret.has_value = false;
            return ret;
        }else
            ret.has_value = true;


        // cur_ already sits at next key (because value parsing consumed its bytes).
        return ret;
    }

    // ========== Utility Operations ==========

    template<std::size_t MAX_SKIP_NESTING, class OutputSinkContainer = void>
    constexpr bool skip_value(OutputSinkContainer* = nullptr,
                                   std::size_t          = std::numeric_limits<std::size_t>::max())
    {
        return skip_one<MAX_SKIP_NESTING>(0);
    }

    bool finish() {
        // CBOR has no insignificant whitespace; if we aren't at the end, it's an error.
        if (cur_ != end_) {
            setError(ParseError::EXCESS_CHARACTERS);
            return false;
        }
        return true;
    }

private:
    It & m_errorPos;
    It & cur_;
    Sent end_;  // Store by value, not reference (avoid dangling reference to temporaries)

    ParseError err_;

    // State for current string (value or key) being streamed.
    // Note: For forward-only iterators, we don't need to store the start position.
    // cur_ always points to the next byte to read, and value_str_offset_ tracks progress.
    std::size_t         value_str_len_    = 0;
    std::size_t         value_str_offset_ = 0;
    bool                value_str_active_ = false;

    // ---- Helpers ----

    void setError(ParseError e) noexcept {
        if (err_ == ParseError::NO_ERROR) {
            err_ = e;
        }
    }

    bool ensure_bytes()  noexcept {
        if(cur_ == end_) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return false;
        }
        else return true;
    }

    void reset_value_string_state() noexcept {
        value_str_len_    = 0;
        value_str_offset_ = 0;
        value_str_active_ = false;
    }

    // Decode "length-like" argument (used for strings / arrays / maps).
    bool decode_length(std::uint8_t ai, std::uint64_t& len) {
        if (ai < 24) {
            ++cur_;
            len = ai;
            return true;
        }

        if (ai == 24) {
            if (!ensure_bytes()) return false;
            ++cur_;
            if (!ensure_bytes()) return false;
            len = *cur_;
            ++cur_;
            return true;
        }

        if (ai == 25) {
            if (!ensure_bytes()) return false;
            ++cur_;

            std::uint16_t v = 0;
            for(int i = 0; i < 2; i ++) {
                if (!ensure_bytes()) return false;
                v |= (static_cast<std::uint16_t>(*cur_) << (8 * (1-i)));
                ++ cur_;
            }
            len = v;
            return true;
        }

        if (ai == 26) {
            if (!ensure_bytes()) return false;
            ++cur_;

            std::uint32_t v = 0;
            for(int i = 0; i < 4; i ++) {
                if (!ensure_bytes()) return false;
                v |= (static_cast<std::uint32_t>(*cur_) << (8 * (3-i)));
                ++ cur_;
            }

            len = v;
            return true;
        }

        if (ai == 27) {
            if (!ensure_bytes()) return false;
            ++cur_;

            std::uint64_t v = 0;
            for(int i = 0; i < 8; i ++) {
                if (!ensure_bytes()) return false;
                v |= (static_cast<std::uint64_t>(*cur_) << (8 * (7-i)));  // Fixed: uint64_t
                ++ cur_;

            }

            len = v;
            return true;
        }

        // Indefinite length or unknown: not supported in v1
        setError(ParseError::NOT_IMPLEMENTED);
        return false;
    }

    // Decode major-type 0/1 integer payload (u64) and advance cur_.
    bool decode_uint(std::uint8_t ai, std::uint64_t& out) {
        if (ai < 24) {
            out = ai;
            ++cur_;
            return true;
        }

        if (ai == 24) {
            if (!ensure_bytes()) return false;
            ++cur_;
            if (!ensure_bytes()) return false;
            out = *cur_;
            ++cur_;

            return true;
        }

        if (ai == 25) {
            if (!ensure_bytes()) return false;
            ++cur_;
            std::uint16_t v = 0;
            for(int i = 0; i < 2; i ++) {
                if (!ensure_bytes()) return false;
                v |= (static_cast<std::uint16_t>(*cur_) << (8 * (1-i)));
                ++ cur_;
            }
            out = v;
            return true;
        }

        if (ai == 26) {
            if (!ensure_bytes()) return false;
            ++cur_;

            std::uint32_t v = 0;
            for(int i = 0; i < 4; i ++) {
                if (!ensure_bytes()) return false;
                v |= (static_cast<std::uint32_t>(*cur_) << (8 * (3-i)));
                ++ cur_;
            }

            out = v;
            return true;
        }

        if (ai == 27) {
            if (!ensure_bytes()) return false;
            ++cur_;

            std::uint64_t v = 0;
            for(int i = 0; i < 8; i ++) {
                if (!ensure_bytes()) return false;
                v |= (static_cast<std::uint64_t>(*cur_) << (8 * (7-i)));  // Fixed: uint64_t
                ++ cur_;
            }

            out = v;
            return true;
        }

        setError(ParseError::ILLFORMED_NUMBER);
        return false;
    }

    bool decode_float(std::uint8_t ai, double& out) {
        if (ai == 25) { // half-precision
            if (!ensure_bytes()) return false;
            ++cur_;
            std::uint16_t v = 0;
            for(int i = 0; i < 2; i ++) {
                if (!ensure_bytes()) return false;
                v |= (static_cast<std::uint16_t>(*cur_) << (8 * (1-i)));
                ++ cur_;
            }
            out = half_to_double(v);
            return true;
        }

        if (ai == 26) { // float32
            if (!ensure_bytes()) return false;
            ++cur_;

            std::uint32_t v = 0;
            for(int i = 0; i < 4; i ++) {
                if (!ensure_bytes()) return false;
                v |= (static_cast<std::uint32_t>(*cur_) << (8 * (3-i)));
                ++ cur_;
            }

            float f;
            std::memcpy(&f, &v, sizeof(f));
            out = static_cast<double>(f);
            return true;
        }

        if (ai == 27) { // float64
            if (!ensure_bytes()) return false;
            ++cur_;

            std::uint64_t v = 0;
            for(int i = 0; i < 8; i ++) {
                if (!ensure_bytes()) return false;
                v |= (static_cast<std::uint64_t>(*cur_) << (8 * (7-i)));  // Fixed: uint64_t
                ++ cur_;
            }

            double d;
            std::memcpy(&d, &v, sizeof(d));
            out = d;
            return true;
        }

        setError(ParseError::ILLFORMED_NUMBER);
        return false;
    }

    static double half_to_double(std::uint16_t h) {
        std::uint16_t sign = (h >> 15) & 0x1;
        std::uint16_t exp  = (h >> 10) & 0x1F;
        std::uint16_t frac = h & 0x3FF;

        if (exp == 0) {
            if (frac == 0) {
                return sign ? -0.0 : 0.0;
            }
            double v = static_cast<double>(frac) / (1 << 10);
            double result = std::ldexp(v, -14); // 2^(-14)
            return sign ? -result : result;
        } else if (exp == 0x1F) {
            // Inf / NaN. For now we just map to infinities; you can tighten this if needed.
            return sign ? -std::numeric_limits<double>::infinity()
                        :  std::numeric_limits<double>::infinity();
        } else {
            double v = 1.0 + static_cast<double>(frac) / (1 << 10);
            int e = static_cast<int>(exp) - 15; // bias 15
            double result = std::ldexp(v, e);
            return sign ? -result : result;
        }
    }

    template<std::size_t MAX_SKIP_NESTING>
    bool skip_one(std::size_t depth) {
        if (depth > MAX_SKIP_NESTING) {
            setError(ParseError::SKIPPING_STACK_OVERFLOW);
            return false;
        }

        if (!ensure_bytes()) {
            return false;
        }

        const std::uint8_t ib = *cur_;
        const std::uint8_t major = ib >> 5;
        const std::uint8_t ai    = ib & 0x1F;

        switch (major) {
        case 0: // unsigned int
        case 1: { // negative int
            std::uint64_t dummy;
            return decode_uint(ai, dummy);
        }

        case 2: // byte string
        case 3: { // text string
            std::uint64_t len;
            if (!decode_length(ai, len)) {
                return false;
            }
            for(std::uint64_t i = 0; i < len; i ++) {  // Fixed: uint64_t
                if (!ensure_bytes()) return false;
                ++cur_;
            }

            return true;
        }

        case 4: { // array
            std::uint64_t len;
            if (!decode_length(ai, len)) {
                return false;
            }
            for (std::uint64_t i = 0; i < len; ++i) {
                if (!skip_one<MAX_SKIP_NESTING>(depth + 1)) {
                    return false;
                }
            }
            return true;
        }

        case 5: { // map
            std::uint64_t len;
            if (!decode_length(ai, len)) {
                return false;
            }
            for (std::uint64_t i = 0; i < len; ++i) {
                if (!skip_one<MAX_SKIP_NESTING>(depth + 1)) return false; // key
                if (!skip_one<MAX_SKIP_NESTING>(depth + 1)) return false; // value
            }
            return true;
        }

        case 6: // tag – not supported in v1
            setError(ParseError::NOT_IMPLEMENTED);
            return false;

        case 7: { // simple / float / break
            if (ai <= 23) {
                // simple value without payload
                ++cur_;
                return true;
            }
            if (ai == 24) {
                if (!ensure_bytes()) return false;
                ++cur_;
                if (!ensure_bytes()) return false;
                ++cur_;
                return true;
            }
            if (ai == 25 || ai == 26 || ai == 27) {
                double dummy;
                return decode_float(ai, dummy);
            }
            if (ai == 31) {
                // "break" for indefinite items — unsupported here
                setError(ParseError::NOT_IMPLEMENTED);
                return false;
            }
            setError(ParseError::ILLFORMED_VALUE);
            return false;
        }
        }

        setError(ParseError::SKIP_ERROR);
        return false;
    }
};

static_assert(reader::ReaderLike<CborReader<std::uint8_t*, std::uint8_t*>>);

template<class It, class Sent>
class CborWriter {
public:
    enum class CborWriterError {
        none,
        not_implemented,
        invalid_argument,
        sink_error
    };
    using iterator_type = It;
    using error_type    = CborWriterError;

    struct ArrayFrame {
        std::size_t expected_size = 0;  // number of elements (unused if indefinite)
        std::size_t written       = 0;  // number of elements actually written
        bool        indefinite    = false;  // true if indefinite-length array
    };

    struct MapFrame {
        std::size_t expected_pairs   = 0; // number of key/value pairs (unused if indefinite)
        std::size_t written_pairs    = 0; // completed pairs
        bool        expecting_key    = true; // for sanity checking
        bool        indefinite       = false; // true if indefinite-length map
    };

    constexpr It & current() {
        return m_current;
    }

    explicit CborWriter(It & first, Sent last) noexcept
        : m_errorPos(first), m_current(first), end_(last)
        , err_(CborWriterError::none)
    {}

    // ========= Introspection =========

    error_type getError() const noexcept { return err_; }

    // ========= Containers =========

    bool write_array_begin(const std::size_t& size, ArrayFrame& frame) {
        if (size == std::numeric_limits<std::size_t>::max()) {
            // Indefinite-length array (0x9F = major 4, ai 31)
            if (!write_byte(0x9F)) {
                return false;
            }
            frame.indefinite = true;
            frame.expected_size = 0;  // Unused
            frame.written = 0;
            return true;
        }

        // Definite-length array
        if (!write_major_type_with_length(/*major=*/4, size)) {
            return false;
        }

        frame.indefinite = false;
        frame.expected_size = size;
        frame.written       = 0;
        return true;
    }

    bool write_array_end(ArrayFrame& frame) {
        if (frame.indefinite) {
            // Write break marker (0xFF = major 7, ai 31)
            return write_byte(0xFF);
        }

        // For definite-length arrays there is no terminator byte in CBOR.
        // Optionally enforce that we wrote exactly expected_size elements.
        if(frame.expected_size < 2 && frame.written != 0) {
            setError(CborWriterError::invalid_argument);
            return false;
        }

        if (frame.expected_size >= 2 && frame.written != frame.expected_size - 1) {
            setError(CborWriterError::invalid_argument);
            return false;
        }
        return true;
    }

    bool write_map_begin(const std::size_t& size, MapFrame& frame) {
        if (size == std::numeric_limits<std::size_t>::max()) {
            // Indefinite-length map (0xBF = major 5, ai 31)
            if (!write_byte(0xBF)) {
                return false;
            }
            frame.indefinite = true;
            frame.expected_pairs = 0;  // Unused
            frame.written_pairs = 0;
            frame.expecting_key = true;
            return true;
        }

        // Definite-length map
        if (!write_major_type_with_length(/*major=*/5, size)) {
            return false;
        }

        frame.indefinite = false;
        frame.expected_pairs = size;
        frame.written_pairs  = 0;
        frame.expecting_key  = true;
        return true;
    }

    bool write_map_end(MapFrame& frame) {
        if (frame.indefinite) {
            // Write break marker (0xFF = major 7, ai 31)
            return write_byte(0xFF);
        }

        // Definite-length validation
        if(frame.expected_pairs < 2 && frame.written_pairs != 0) {
            setError(CborWriterError::invalid_argument);
            return false;
        }
        if (frame.expected_pairs >= 2 && frame.written_pairs != frame.expected_pairs - 1) {
            setError(CborWriterError::invalid_argument);
            return false;
        }
        return true;
    }

    // After finishing an array element / a map value
    bool advance_after_value(ArrayFrame& frame) {
        if (!frame.indefinite && frame.written >= frame.expected_size) {
            setError(CborWriterError::invalid_argument);
            return false;
        }
        ++frame.written;
        return true;
    }

    bool advance_after_value(MapFrame& frame) {
        // We should have just written a *value*, not a key
        if (frame.expecting_key) {
            setError(CborWriterError::invalid_argument);
            return false;
        }
        if (!frame.indefinite && frame.written_pairs >= frame.expected_pairs) {
            setError(CborWriterError::invalid_argument);
            return false;
        }
        ++frame.written_pairs;
        frame.expecting_key = true;
        return true;
    }

    // Move from key → value within a map
    bool move_to_value(MapFrame& frame) {
        if (!frame.expecting_key) {
            // We were already at value; misused API
            setError(CborWriterError::invalid_argument);
            return false;
        }
        frame.expecting_key = false;
        // For CBOR, keys and values are just sequential; nothing to encode here.
        return true;
    }

    // For indexes_as_keys: write `index` as a CBOR integer key.
    bool write_key_as_index(const std::size_t& idx) {
        return write_number(idx);
    }

    // ========= Primitive values =========

    bool write_null() {
        // CBOR simple value "null" is major 7, additional info 22 => 0xF6
        return write_byte(0xF6);
    }

    bool write_bool(const bool& b) {
        // false: 0xF4, true: 0xF5
        return write_byte(b ? 0xF5 : 0xF4);
    }

    template<class NumberT>
    bool write_number(const NumberT& n) {
        if constexpr (std::is_integral_v<NumberT>) {
            return write_integral(n);
        }  else if constexpr (std::is_floating_point_v<NumberT>) {
            if constexpr (std::is_same_v<NumberT, float>) {
                return write_float32(n);
            } else {
                // double / long double → 64-bit for now
                return write_float64(static_cast<double>(n));
            }
        } else {
            static_assert(std::is_integral_v<NumberT> || std::is_floating_point_v<NumberT>,
                          "CborWriter::write_number only supports integral or floating point types");
        }
    }

    bool write_string(const char* data, std::size_t size, bool null_terminated = false) {
        // Major type 3 = text string, argument = length in bytes
        if(null_terminated) {
            size = 0;
            const char * d = data;
            while(*d != '\0') {  // Fixed: dereference pointer
                d ++;
                size ++;
            }
        }
        if (!write_major_type_with_length(/*major=*/3, size)) {
            return false;
        }
        return write_bytes(reinterpret_cast<const std::uint8_t*>(data), size);
    }

    // ========= Finalization =========

    bool finish() {
        // Nothing to flush for this simple writer.
        return (err_ == CborWriterError::none);
    }

private:
    It & m_errorPos;
    It & m_current;
    Sent end_;  // Store by value, not reference (avoid dangling reference to temporaries)
    error_type       err_ = CborWriterError::none;

    void setError(error_type e) noexcept {
        if (err_ == CborWriterError::none) {
            err_ = e;
            m_errorPos = m_current;
        }
    }

    bool write_byte(std::uint8_t b) {
        if(m_current == end_) {
            setError(CborWriterError::sink_error);
            return false;
        }
        *m_current ++ = b;
        return true;

    }

    bool write_bytes(const std::uint8_t* data, std::size_t len) {
        // Minimal, generic implementation: push_back in a loop.
        // If OutputContainer supports insert, you can specialize later.

        for (std::size_t i = 0; i < len; ++i) {
            if(m_current == end_) {
                setError(CborWriterError::sink_error);
                return false;
            }
            *m_current ++ = data[i];

        }
        return true;

    }

    // ======== CBOR helpers ========

    bool write_major_type_with_length(std::uint8_t major, std::uint64_t length) {
        // Encode "major type N, argument = length" as per RFC 8949.
        if (length <= 23) {
            // Small argument in low 5 bits.
            std::uint8_t ib = static_cast<std::uint8_t>((major << 5) | static_cast<std::uint8_t>(length));
            return write_byte(ib);
        } else if (length <= 0xFFu) {
            std::uint8_t ib = static_cast<std::uint8_t>((major << 5) | 24u);
            if (!write_byte(ib)) return false;
            std::uint8_t b = static_cast<std::uint8_t>(length);
            return write_byte(b);
        } else if (length <= 0xFFFFu) {
            std::uint8_t ib = static_cast<std::uint8_t>((major << 5) | 25u);
            if (!write_byte(ib)) return false;
            std::uint16_t v = static_cast<std::uint16_t>(length);
            std::uint8_t buf[2] = {
                static_cast<std::uint8_t>((v >> 8) & 0xFFu),
                static_cast<std::uint8_t>(v & 0xFFu)
            };
            return write_bytes(buf, 2);
        } else if (length <= 0xFFFFFFFFu) {
            std::uint8_t ib = static_cast<std::uint8_t>((major << 5) | 26u);
            if (!write_byte(ib)) return false;
            std::uint32_t v = static_cast<std::uint32_t>(length);
            std::uint8_t buf[4] = {
                static_cast<std::uint8_t>((v >> 24) & 0xFFu),
                static_cast<std::uint8_t>((v >> 16) & 0xFFu),
                static_cast<std::uint8_t>((v >> 8)  & 0xFFu),
                static_cast<std::uint8_t>(v & 0xFFu)
            };
            return write_bytes(buf, 4);
        } else {
            std::uint8_t ib = static_cast<std::uint8_t>((major << 5) | 27u);
            if (!write_byte(ib)) return false;
            std::uint64_t v = length;
            std::uint8_t buf[8];
            for (int i = 0; i < 8; ++i) {
                buf[7 - i] = static_cast<std::uint8_t>((v >> (8 * i)) & 0xFFu);
            }
            return write_bytes(buf, 8);
        }
    }

    template<class Int>
    bool write_integral(Int value) {
        static_assert(std::is_integral_v<Int>, "write_integral requires integral type");

        using Signed   = std::conditional_t<std::is_signed_v<Int>, Int, std::int64_t>;
        // using Unsigned = std::conditional_t<std::is_unsigned_v<Int>, Int, std::uint64_t>;

        if constexpr (std::is_signed_v<Int>) {
            if (value >= 0) {
                return write_unsigned(static_cast<std::uint64_t>(value));
            } else {
                // CBOR negative integers: -1 - n encoded in major type 1.
                std::int64_t v = static_cast<Signed>(value);
                std::uint64_t n = static_cast<std::uint64_t>(-1 - v);
                return write_negative(n);
            }
        } else {
            return write_unsigned(static_cast<std::uint64_t>(value));
        }
    }

    bool write_unsigned(std::uint64_t v) {
        // major type 0
        return write_major_type_with_uint(/*major=*/0, v);
    }

    bool write_negative(std::uint64_t n) {
        // major type 1, n encodes -1 - n
        return write_major_type_with_uint(/*major=*/1, n);
    }

    bool write_major_type_with_uint(std::uint8_t major, std::uint64_t v) {
        if (v <= 23u) {
            std::uint8_t ib = static_cast<std::uint8_t>((major << 5) | static_cast<std::uint8_t>(v));
            return write_byte(ib);
        } else if (v <= 0xFFu) {
            std::uint8_t ib = static_cast<std::uint8_t>((major << 5) | 24u);
            if (!write_byte(ib)) return false;
            std::uint8_t b = static_cast<std::uint8_t>(v);
            return write_byte(b);
        } else if (v <= 0xFFFFu) {
            std::uint8_t ib = static_cast<std::uint8_t>((major << 5) | 25u);
            if (!write_byte(ib)) return false;
            std::uint16_t s = static_cast<std::uint16_t>(v);
            std::uint8_t buf[2] = {
                static_cast<std::uint8_t>((s >> 8) & 0xFFu),
                static_cast<std::uint8_t>(s & 0xFFu)
            };
            return write_bytes(buf, 2);
        } else if (v <= 0xFFFFFFFFu) {
            std::uint8_t ib = static_cast<std::uint8_t>((major << 5) | 26u);
            if (!write_byte(ib)) return false;
            std::uint32_t w = static_cast<std::uint32_t>(v);
            std::uint8_t buf[4] = {
                static_cast<std::uint8_t>((w >> 24) & 0xFFu),
                static_cast<std::uint8_t>((w >> 16) & 0xFFu),
                static_cast<std::uint8_t>((w >> 8)  & 0xFFu),
                static_cast<std::uint8_t>(w & 0xFFu)
            };
            return write_bytes(buf, 4);
        } else {
            std::uint8_t ib = static_cast<std::uint8_t>((major << 5) | 27u);
            if (!write_byte(ib)) return false;
            std::uint64_t x = v;
            std::uint8_t buf[8];
            for (int i = 0; i < 8; ++i) {
                buf[7 - i] = static_cast<std::uint8_t>((x >> (8 * i)) & 0xFFu);
            }
            return write_bytes(buf, 8);
        }
    }

    bool write_float32(float f) {
        std::uint8_t ib = static_cast<std::uint8_t>((7u << 5) | 26u); // 0xFA
        if (!write_byte(ib)) return false;

        static_assert(sizeof(float) == 4);
        std::uint32_t bits;
        std::memcpy(&bits, &f, sizeof(float));

        std::uint8_t buf[4] = {
            static_cast<std::uint8_t>((bits >> 24) & 0xFFu),
            static_cast<std::uint8_t>((bits >> 16) & 0xFFu),
            static_cast<std::uint8_t>((bits >> 8)  & 0xFFu),
            static_cast<std::uint8_t>(bits & 0xFFu)
        };
        return write_bytes(buf, 4);
    }

    bool write_float64(double d) {
        std::uint8_t ib = static_cast<std::uint8_t>((7u << 5) | 27u); // 0xFB
        if (!write_byte(ib)) return false;

        static_assert(sizeof(double) == 8);
        std::uint64_t bits;
        std::memcpy(&bits, &d, sizeof(double));

        std::uint8_t buf[8];
        for (int i = 0; i < 8; ++i) {
            buf[7 - i] = static_cast<std::uint8_t>((bits >> (8 * i)) & 0xFFu);
        }
        return write_bytes(buf, 8);
    }
};
static_assert(writer::WriterLike<CborWriter<std::uint8_t*, std::uint8_t*>>);

} // namespace JsonFusion
