#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <limits>
#include <cmath>

#include "reader_concept.hpp"   // TryParseStatus, StringChunkResult
#include "parse_errors.hpp"      // ParseError enum

namespace JsonFusion {

class CborReader {
public:
    using iterator_type = const std::uint8_t*;

    struct ArrayFrame {
        std::uint64_t remaining = 0;  // elements left in this array
    };

    struct ObjectFrame {
        std::uint64_t remaining_pairs = 0; // key/value pairs left
        // (we don't need any other state; parser drives key/value phases)
    };

    CborReader(const std::uint8_t* begin,
               const std::uint8_t* end) noexcept
        : begin_(begin)
        , cur_(begin)
        , end_(end)
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
    constexpr json_reader::TryParseStatus skip_ws_and_read_null() {
        if (!ensure_bytes(1)) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return json_reader::TryParseStatus::error;
        }

        std::uint8_t ib = *cur_;
        std::uint8_t major = ib >> 5;
        std::uint8_t ai    = ib & 0x1F;

        if (major == 7 && ai == 22) { // null
            ++cur_;
            return json_reader::TryParseStatus::ok;
        }

        return json_reader::TryParseStatus::no_match;
    }

    constexpr json_reader::TryParseStatus read_bool(bool& b) {
        if (!ensure_bytes(1)) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return json_reader::TryParseStatus::error;
        }

        std::uint8_t ib = *cur_;
        std::uint8_t major = ib >> 5;
        std::uint8_t ai    = ib & 0x1F;

        if (major != 7) {
            return json_reader::TryParseStatus::no_match;
        }

        if (ai == 20) {        // false
            b = false;
            ++cur_;
            return json_reader::TryParseStatus::ok;
        } else if (ai == 21) { // true
            b = true;
            ++cur_;
            return json_reader::TryParseStatus::ok;
        }

        return json_reader::TryParseStatus::no_match;
    }

    template<class NumberT>
    constexpr json_reader::TryParseStatus read_number(NumberT& storage) {
        static_assert(std::is_integral_v<NumberT> || std::is_floating_point_v<NumberT>,
                      "CborReader::read_number requires integral or floating type");

        if (!ensure_bytes(1)) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return json_reader::TryParseStatus::error;
        }

        const std::uint8_t ib = *cur_;
        const std::uint8_t major = ib >> 5;
        const std::uint8_t ai    = ib & 0x1F;

        // ----- Integral storage -----
        if constexpr (std::is_integral_v<NumberT>) {
            if (major == 0) { // unsigned
                std::uint64_t uval;
                if (!decode_uint(ai, uval)) {
                    return json_reader::TryParseStatus::error;
                }

                if (uval > static_cast<std::uint64_t>(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return json_reader::TryParseStatus::error;
                }
                storage = static_cast<NumberT>(uval);
                return json_reader::TryParseStatus::ok;
            } else if (major == 1) { // negative: value = -1 - n
                std::uint64_t n;
                if (!decode_uint(ai, n)) {
                    return json_reader::TryParseStatus::error;
                }
                std::int64_t sval = -static_cast<std::int64_t>(n) - 1;

                if constexpr (std::is_unsigned_v<NumberT>) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return json_reader::TryParseStatus::error;
                } else {
                    if (sval < std::numeric_limits<NumberT>::lowest() ||
                        sval > std::numeric_limits<NumberT>::max()) {
                        setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                        return json_reader::TryParseStatus::error;
                    }
                    storage = static_cast<NumberT>(sval);
                    return json_reader::TryParseStatus::ok;
                }
            }

            // allow float encodings -> integer if they fit?
            if (major == 7 && (ai == 25 || ai == 26 || ai == 27)) {
                double dv{};
                if (!decode_float(ai, dv)) {
                    return json_reader::TryParseStatus::error;
                }
                if (dv < static_cast<double>(std::numeric_limits<NumberT>::lowest()) ||
                    dv > static_cast<double>(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return json_reader::TryParseStatus::error;
                }
                storage = static_cast<NumberT>(dv);
                return json_reader::TryParseStatus::ok;
            }

            return json_reader::TryParseStatus::no_match;
        }

        // ----- Floating storage -----
        if constexpr (std::is_floating_point_v<NumberT>) {
            if (major == 7 && (ai == 25 || ai == 26 || ai == 27)) {
                double dv{};
                if (!decode_float(ai, dv)) {
                    return json_reader::TryParseStatus::error;
                }
                if (dv < static_cast<double>(std::numeric_limits<NumberT>::lowest()) ||
                    dv > static_cast<double>(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return json_reader::TryParseStatus::error;
                }
                storage = static_cast<NumberT>(dv);
                return json_reader::TryParseStatus::ok;
            }

            // int → float is allowed
            if (major == 0) {
                std::uint64_t uval;
                if (!decode_uint(ai, uval)) {
                    return json_reader::TryParseStatus::error;
                }
                double dv = static_cast<double>(uval);
                if (dv < static_cast<double>(std::numeric_limits<NumberT>::lowest()) ||
                    dv > static_cast<double>(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return json_reader::TryParseStatus::error;
                }
                storage = static_cast<NumberT>(dv);
                return json_reader::TryParseStatus::ok;
            } else if (major == 1) {
                std::uint64_t n;
                if (!decode_uint(ai, n)) {
                    return json_reader::TryParseStatus::error;
                }
                double dv = -static_cast<double>(n) - 1.0;
                if (dv < static_cast<double>(std::numeric_limits<NumberT>::lowest()) ||
                    dv > static_cast<double>(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return json_reader::TryParseStatus::error;
                }
                storage = static_cast<NumberT>(dv);
                return json_reader::TryParseStatus::ok;
            }

            return json_reader::TryParseStatus::no_match;
        }
    }

    // ========== String parsing (chunked) ==========

    // Parser calls this for:
    //   - JSON-like "string values",
    //   - JSON-like "object keys" (for struct field lookup / maps).
    constexpr json_reader::StringChunkResult read_string_chunk(char* out,
                                                               std::size_t capacity) {
        json_reader::StringChunkResult res{};
        res.status        = json_reader::StringChunkStatus::error;
        res.bytes_written = 0;
        res.done          = false;

        if (capacity == 0) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return res;
        }

        if (!value_str_active_) {
            // First call for this string.
            if (!ensure_bytes(1)) {
                setError(ParseError::UNEXPECTED_END_OF_DATA);
                return res;
            }

            const std::uint8_t ib = *cur_;
            const std::uint8_t major = ib >> 5;
            const std::uint8_t ai    = ib & 0x1F;

            // Treat both text (major 3) and byte string (major 2) as "string-like".
            if (major != 2 && major != 3) {
                res.status = json_reader::StringChunkStatus::no_match;
                return res;
            }

            std::uint64_t len;
            if (!decode_length(ai, len)) {
                // decode_length sets error on failure.
                return res;
            }

            if (!ensure_bytes(static_cast<std::size_t>(len))) {
                setError(ParseError::UNEXPECTED_END_OF_DATA);
                return res;
            }

            value_str_data_   = cur_;
            value_str_len_    = static_cast<std::size_t>(len);
            value_str_offset_ = 0;
            value_str_active_ = true;
        }

        const std::size_t remaining = value_str_len_ - value_str_offset_;
        const std::size_t n         = remaining < capacity ? remaining : capacity;

        std::memcpy(out, value_str_data_ + value_str_offset_, n);
        value_str_offset_ += n;

        res.status        = json_reader::StringChunkStatus::ok;
        res.bytes_written = n;
        res.done          = (value_str_offset_ >= value_str_len_);

        if (res.done) {
            // Advance underlying cursor past the string bytes.
            cur_ = value_str_data_ + value_str_len_;
            reset_value_string_state();
        }

        return res;
    }

    // Index-based keys (for indexes_as_keys).
    constexpr bool read_key_as_index(std::size_t& out) {
        if (!ensure_bytes(1)) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
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

    constexpr bool read_array_begin(ArrayFrame& frame) {
        reset_value_string_state();

        if (!ensure_bytes(1)) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return false;
        }

        const std::uint8_t ib = *cur_;
        const std::uint8_t major = ib >> 5;
        const std::uint8_t ai    = ib & 0x1F;

        if (major != 4) {
            return false;  // not an array here
        }

        std::uint64_t len;
        if (!decode_length(ai, len)) {
            return false;  // error already set
        }

        frame.remaining = len;
        // cur_ now points to the first element (if len > 0) or to the first byte after
        // the array data (if len == 0 and no elements will be read).
        return true;
    }

    constexpr json_reader::TryParseStatus read_array_end(const ArrayFrame& frame) {
        if (frame.remaining > 0) {
            return json_reader::TryParseStatus::no_match;
        }
        return json_reader::TryParseStatus::ok;
    }

    // After each element:
    //   - element has been fully parsed (cur_ is at next element or after array)
    //   - we just decrement remaining, no cursor work needed.
    constexpr bool consume_value_separator(ArrayFrame& frame, bool& had_comma) {
        reset_value_string_state();
        if (frame.remaining == 0) {
            had_comma = false;
            return true;
        }
        --frame.remaining;
        had_comma = (frame.remaining > 0);
        return true;
    }

    // ========== Objects (maps) ==========

    constexpr bool read_object_begin(ObjectFrame& frame) {
        reset_value_string_state();

        if (!ensure_bytes(1)) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return false;
        }

        const std::uint8_t ib = *cur_;
        const std::uint8_t major = ib >> 5;
        const std::uint8_t ai    = ib & 0x1F;

        if (major != 5) {
            return false;  // not a map here
        }

        std::uint64_t len;
        if (!decode_length(ai, len)) {
            return false;  // error already set
        }

        frame.remaining_pairs = len;
        // cur_ now points at the first key (if len > 0) or at first byte after the map.
        return true;
    }

    constexpr json_reader::TryParseStatus read_object_end(const ObjectFrame& frame) {
        if (frame.remaining_pairs > 0) {
            return json_reader::TryParseStatus::no_match;
        }
        return json_reader::TryParseStatus::ok;
    }

    // After key is fully read (via read_string_chunk or read_key_as_index)
    // parser calls consume_kv_separator() to switch to the value.
    // In CBOR, we already advanced cur_ past the key, so it's now at the value.
    constexpr bool consume_kv_separator(ObjectFrame& frame) {
        (void)frame;
        reset_value_string_state();
        // Nothing to do: cur_ already at value.
        return true;
    }

    // After value is parsed, parser calls this to advance to next key.
    constexpr bool consume_value_separator(ObjectFrame& frame, bool& had_comma) {
        reset_value_string_state();
        if (frame.remaining_pairs == 0) {
            had_comma = false;
            return true;
        }

        --frame.remaining_pairs;
        had_comma = (frame.remaining_pairs > 0);
        // cur_ already sits at next key (because value parsing consumed its bytes).
        return true;
    }

    // ========== Utility Operations ==========

    template<std::size_t MAX_SKIP_NESTING, class OutputSinkContainer = void>
    constexpr bool skip_json_value(OutputSinkContainer* = nullptr,
                                   std::size_t          = std::numeric_limits<std::size_t>::max())
    {
        return skip_one<MAX_SKIP_NESTING>(0);
    }

    bool skip_whitespaces_till_the_end() {
        // CBOR has no insignificant whitespace; if we aren't at the end, it's an error.
        if (cur_ != end_) {
            setError(ParseError::ILLFORMED_OBJECT);
            return false;
        }
        return true;
    }

private:
    const std::uint8_t* begin_ = nullptr;
    const std::uint8_t* cur_   = nullptr;
    const std::uint8_t* end_   = nullptr;

    ParseError err_;

    // State for current string (value or key) being streamed.
    const std::uint8_t* value_str_data_   = nullptr;
    std::size_t         value_str_len_    = 0;
    std::size_t         value_str_offset_ = 0;
    bool                value_str_active_ = false;

    // ---- Helpers ----

    void setError(ParseError e) noexcept {
        if (err_ == ParseError::NO_ERROR) {
            err_ = e;
        }
    }

    bool ensure_bytes(std::size_t n) const noexcept {
        return static_cast<std::size_t>(end_ - cur_) >= n;
    }

    void reset_value_string_state() noexcept {
        value_str_data_   = nullptr;
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
            if (!ensure_bytes(2)) {
                setError(ParseError::UNEXPECTED_END_OF_DATA);
                return false;
            }
            ++cur_;
            len = *cur_;
            ++cur_;
            return true;
        }

        if (ai == 25) {
            if (!ensure_bytes(3)) {
                setError(ParseError::UNEXPECTED_END_OF_DATA);
                return false;
            }
            ++cur_;
            std::uint16_t v = (static_cast<std::uint16_t>(cur_[0]) << 8) |
                              (static_cast<std::uint16_t>(cur_[1])     );
            cur_ += 2;
            len = v;
            return true;
        }

        if (ai == 26) {
            if (!ensure_bytes(5)) {
                setError(ParseError::UNEXPECTED_END_OF_DATA);
                return false;
            }
            ++cur_;
            std::uint32_t v =
                (static_cast<std::uint32_t>(cur_[0]) << 24) |
                (static_cast<std::uint32_t>(cur_[1]) << 16) |
                (static_cast<std::uint32_t>(cur_[2]) << 8)  |
                (static_cast<std::uint32_t>(cur_[3])      );
            cur_ += 4;
            len = v;
            return true;
        }

        if (ai == 27) {
            if (!ensure_bytes(9)) {
                setError(ParseError::UNEXPECTED_END_OF_DATA);
                return false;
            }
            ++cur_;
            std::uint64_t v =
                (static_cast<std::uint64_t>(cur_[0]) << 56) |
                (static_cast<std::uint64_t>(cur_[1]) << 48) |
                (static_cast<std::uint64_t>(cur_[2]) << 40) |
                (static_cast<std::uint64_t>(cur_[3]) << 32) |
                (static_cast<std::uint64_t>(cur_[4]) << 24) |
                (static_cast<std::uint64_t>(cur_[5]) << 16) |
                (static_cast<std::uint64_t>(cur_[6]) << 8)  |
                (static_cast<std::uint64_t>(cur_[7])      );
            cur_ += 8;
            len = v;
            return true;
        }

        // Indefinite length or unknown: not supported in v1
        setError(ParseError::ILLFORMED_OBJECT);
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
            if (!ensure_bytes(2)) {
                setError(ParseError::UNEXPECTED_END_OF_DATA);
                return false;
            }
            ++cur_;
            out = *cur_;
            ++cur_;
            return true;
        }

        if (ai == 25) {
            if (!ensure_bytes(3)) {
                setError(ParseError::UNEXPECTED_END_OF_DATA);
                return false;
            }
            ++cur_;
            std::uint16_t v = (static_cast<std::uint16_t>(cur_[0]) << 8) |
                              (static_cast<std::uint16_t>(cur_[1])     );
            cur_ += 2;
            out = v;
            return true;
        }

        if (ai == 26) {
            if (!ensure_bytes(5)) {
                setError(ParseError::UNEXPECTED_END_OF_DATA);
                return false;
            }
            ++cur_;
            std::uint32_t v =
                (static_cast<std::uint32_t>(cur_[0]) << 24) |
                (static_cast<std::uint32_t>(cur_[1]) << 16) |
                (static_cast<std::uint32_t>(cur_[2]) << 8)  |
                (static_cast<std::uint32_t>(cur_[3])      );
            cur_ += 4;
            out = v;
            return true;
        }

        if (ai == 27) {
            if (!ensure_bytes(9)) {
                setError(ParseError::UNEXPECTED_END_OF_DATA);
                return false;
            }
            ++cur_;
            std::uint64_t v =
                (static_cast<std::uint64_t>(cur_[0]) << 56) |
                (static_cast<std::uint64_t>(cur_[1]) << 48) |
                (static_cast<std::uint64_t>(cur_[2]) << 40) |
                (static_cast<std::uint64_t>(cur_[3]) << 32) |
                (static_cast<std::uint64_t>(cur_[4]) << 24) |
                (static_cast<std::uint64_t>(cur_[5]) << 16) |
                (static_cast<std::uint64_t>(cur_[6]) << 8)  |
                (static_cast<std::uint64_t>(cur_[7])      );
            cur_ += 8;
            out = v;
            return true;
        }

        setError(ParseError::ILLFORMED_OBJECT);
        return false;
    }

    bool decode_float(std::uint8_t ai, double& out) {
        if (ai == 25) { // half-precision
            if (!ensure_bytes(3)) {
                setError(ParseError::UNEXPECTED_END_OF_DATA);
                return false;
            }
            ++cur_;
            std::uint16_t h = (static_cast<std::uint16_t>(cur_[0]) << 8) |
                              (static_cast<std::uint16_t>(cur_[1])     );
            cur_ += 2;
            out = half_to_double(h);
            return true;
        }

        if (ai == 26) { // float32
            if (!ensure_bytes(5)) {
                setError(ParseError::UNEXPECTED_END_OF_DATA);
                return false;
            }
            ++cur_;
            std::uint32_t u =
                (static_cast<std::uint32_t>(cur_[0]) << 24) |
                (static_cast<std::uint32_t>(cur_[1]) << 16) |
                (static_cast<std::uint32_t>(cur_[2]) << 8)  |
                (static_cast<std::uint32_t>(cur_[3])      );
            cur_ += 4;
            float f;
            std::memcpy(&f, &u, sizeof(f));
            out = static_cast<double>(f);
            return true;
        }

        if (ai == 27) { // float64
            if (!ensure_bytes(9)) {
                setError(ParseError::UNEXPECTED_END_OF_DATA);
                return false;
            }
            ++cur_;
            std::uint64_t u =
                (static_cast<std::uint64_t>(cur_[0]) << 56) |
                (static_cast<std::uint64_t>(cur_[1]) << 48) |
                (static_cast<std::uint64_t>(cur_[2]) << 40) |
                (static_cast<std::uint64_t>(cur_[3]) << 32) |
                (static_cast<std::uint64_t>(cur_[4]) << 24) |
                (static_cast<std::uint64_t>(cur_[5]) << 16) |
                (static_cast<std::uint64_t>(cur_[6]) << 8)  |
                (static_cast<std::uint64_t>(cur_[7])      );
            cur_ += 8;
            double d;
            std::memcpy(&d, &u, sizeof(d));
            out = d;
            return true;
        }

        setError(ParseError::ILLFORMED_OBJECT);
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

        if (!ensure_bytes(1)) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
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
            if (!ensure_bytes(static_cast<std::size_t>(len))) {
                setError(ParseError::UNEXPECTED_END_OF_DATA);
                return false;
            }
            cur_ += len;
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
            setError(ParseError::ILLFORMED_OBJECT);
            return false;

        case 7: { // simple / float / break
            if (ai <= 23) {
                // simple value without payload
                ++cur_;
                return true;
            }
            if (ai == 24) {
                if (!ensure_bytes(2)) {
                    setError(ParseError::UNEXPECTED_END_OF_DATA);
                    return false;
                }
                cur_ += 2;
                return true;
            }
            if (ai == 25 || ai == 26 || ai == 27) {
                double dummy;
                return decode_float(ai, dummy);
            }
            if (ai == 31) {
                // "break" for indefinite items — unsupported here
                setError(ParseError::ILLFORMED_OBJECT);
                return false;
            }
            setError(ParseError::ILLFORMED_OBJECT);
            return false;
        }
        }

        setError(ParseError::ILLFORMED_OBJECT);
        return false;
    }
};

} // namespace JsonFusion
