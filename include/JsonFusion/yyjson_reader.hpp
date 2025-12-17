#include <yyjson/yyjson.h>
#include <cstring>
#include <vector>
#include <cstdint>
#include <charconv>
#include "reader_concept.hpp"

namespace JsonFusion {

class YyjsonReader {
public:
    using iterator_type = yyjson_val*; // to satisfy Parser's current() API

    // Per-container state lives here, not in the reader.
    struct ArrayFrame {
        yyjson_val*   arr      = nullptr;
        yyjson_arr_iter it{};
        std::size_t   index    = 0;     // current element index
        std::size_t   size     = 0;     // total elements
        yyjson_val*   current  = nullptr; // current element, or nullptr if empty/done
    };

    struct ObjectFrame {
        yyjson_val*   obj      = nullptr;
        yyjson_obj_iter it{};
        yyjson_val*   key      = nullptr; // current key node
        yyjson_val*   value    = nullptr; // current value node
        bool          exhausted = false;  // true when no more members
    };

    explicit YyjsonReader(yyjson_val* root) noexcept
        : root_(root)
        , current_(root)
        , err_(ParseError::NO_ERROR)
    {}

    // ---- Introspection ----

    iterator_type current() const noexcept {
        // For yyjson backend we don't know exact char position; use node ptr.
        return current_;
    }

    ParseError getError() const noexcept { return err_; }

    // ---- Scalars ----

    constexpr json_reader::TryParseStatus skip_ws_and_read_null() {
        if (!current_) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return json_reader::TryParseStatus::error;
        }
        if (!yyjson_is_null(current_)) {
            return json_reader::TryParseStatus::no_match;
        }
        return json_reader::TryParseStatus::ok;
    }

    constexpr json_reader::TryParseStatus read_bool(bool& b) {
        if (!current_) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return json_reader::TryParseStatus::error;
        }
        if (!yyjson_is_bool(current_)) {
            return json_reader::TryParseStatus::no_match;
        }
        b = yyjson_get_bool(current_) != 0;
        return json_reader::TryParseStatus::ok;
    }

    template<class NumberT>
    constexpr json_reader::TryParseStatus read_number(NumberT& storage) {
        if (!current_) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return json_reader::TryParseStatus::error;
        }
        if (!yyjson_is_num(current_)) {
            return json_reader::TryParseStatus::no_match;
        }


        if constexpr (std::is_integral_v<NumberT>) {
            if (yyjson_is_int(current_)) {
                auto v = yyjson_get_sint(current_);
                if (v < std::int64_t(std::numeric_limits<NumberT>::lowest()) ||
                    v > std::int64_t(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return json_reader::TryParseStatus::error;
                }
                storage = static_cast<NumberT>(v);
            } else if (yyjson_is_uint(current_)) {
                auto v = yyjson_get_uint(current_);
                if (v < std::uint64_t(std::numeric_limits<NumberT>::lowest()) ||
                    v > std::uint64_t(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return json_reader::TryParseStatus::error;
                }
                storage = static_cast<NumberT>(v);
            } else {
                auto v = yyjson_get_real(current_);
                if (v < double(std::numeric_limits<NumberT>::lowest()) ||
                    v > double(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return json_reader::TryParseStatus::error;
                }
                storage = static_cast<NumberT>(v);
            }
        } else {
            static_assert(std::is_floating_point_v<NumberT>,
                          "read_number only supports integral or floating types");

            double v;
            if (yyjson_is_real(current_)) {
                v = yyjson_get_real(current_);
            } else if (yyjson_is_sint(current_)) {
                v = static_cast<double>(yyjson_get_sint(current_));
            } else {
                v = static_cast<double>(yyjson_get_uint(current_));
            }

            if (v < double(std::numeric_limits<NumberT>::lowest()) ||
                v > double(std::numeric_limits<NumberT>::max())) {
                setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                return json_reader::TryParseStatus::error;
            }
            storage = static_cast<NumberT>(v);
        }
        return json_reader::TryParseStatus::ok;

    }

    // ---- String reader (for both keys and values) ----
    //
    // Protocol:
    //  - For keys: current_ must point to the key node.
    //  - For values: current_ must point to the value node.
    //  The parser / object frame logic is responsible for setting current_.

    constexpr json_reader::StringChunkResult read_string_chunk(char* out, std::size_t capacity) {
        json_reader::StringChunkResult res{};
        res.status = json_reader::StringChunkStatus::error;
        res.bytes_written = 0;
        res.done = false;

        if (capacity == 0) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return res;
        }

        if (!value_str_active_) {
            if (!current_ || !yyjson_is_str(current_)) {
                res.status = json_reader::StringChunkStatus::no_match;
                return res;
            }

            value_str_data_   = yyjson_get_str(current_);
            value_str_len_    = yyjson_get_len(current_);
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
            reset_value_string_state();
        }

        return res;
    }

    constexpr bool read_key_as_index(std::size_t & out) {
        constexpr std::size_t bufSize = 32;

        char buf[bufSize];
        if(json_reader::StringChunkResult r = read_string_chunk(buf, bufSize-1); !r.done || r.status != json_reader::StringChunkStatus::ok) {
            return false;
        } else {
            buf[r.bytes_written] = 0;
            auto [ptr, ec] = std::from_chars(buf, buf+r.bytes_written, out);
            if(ec != std::errc()) {
                setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                return false;
            } else {
                return true;
            }
        }
    }

    // ---- Arrays (new frame-based API) ----

    // Parser: creates ArrayFrame on its stack and calls this.
    constexpr bool read_array_begin(ArrayFrame& frame) {
        reset_value_string_state();

        if (!current_ || !yyjson_is_arr(current_)) {
            return false;
        }

        yyjson_val* arr = current_;
        frame.arr   = arr;
        frame.size  = yyjson_arr_size(arr);
        frame.index = 0;
        frame.current = nullptr;

        if (frame.size > 0) {
            yyjson_arr_iter_init(arr, &frame.it);
            frame.current = yyjson_arr_iter_next(&frame.it);
            current_ = frame.current; // first element
        } else {
            // Empty array – keep current_ on the array node.
            current_ = frame.arr;
        }

        return true;
    }

    // Parser: polls this in its loop.
    constexpr json_reader::TryParseStatus read_array_end(const ArrayFrame& frame) {
        // If we still have elements left, we're not "at ]" yet.
        if (frame.index < frame.size) {
            return json_reader::TryParseStatus::no_match;
        }
        // All elements consumed.
        return json_reader::TryParseStatus::ok;
    }

    // Parser: call after finishing one element.
    //
    //   bool had_comma = false;
    //   if (!reader.consume_value_separator(frame, had_comma)) error;
    //
    constexpr bool consume_value_separator(ArrayFrame& frame, bool& had_comma) {
        reset_value_string_state();
        had_comma = false;

        if (!frame.arr) {
            // Not in an array context.
            return true;
        }

        ++frame.index;
        if (frame.index < frame.size) {
            // Move to next element.
            frame.current = yyjson_arr_iter_next(&frame.it);
            current_ = frame.current;
            had_comma = true;
        } else {
            // Last element finished; current_ becomes the array node.
            frame.current = nullptr;
            current_ = frame.arr;
            had_comma = false;
        }
        return true;
    }

    // ---- Objects (new frame-based API) ----

    // Parser: creates ObjectFrame on its stack and calls this.
    constexpr bool read_object_begin(ObjectFrame& frame) {
        reset_value_string_state();

        if (!current_ || !yyjson_is_obj(current_)) {
            return false;
        }

        yyjson_val* obj = current_;
        frame.obj       = obj;
        frame.key       = nullptr;
        frame.value     = nullptr;
        frame.exhausted = false;

        yyjson_obj_iter_init(obj, &frame.it);

        // Preload first member, if any.
        if (!advance_object_member(frame)) {
            // empty object: current_ remains on object, exhausted=true
            current_ = frame.obj;
        } else {
            // we have a first key; set current_ to that key for key-string reading.
            current_ = frame.key;
        }

        return true;
    }

    constexpr json_reader::TryParseStatus read_object_end(const ObjectFrame& frame) {
        if (!frame.obj) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return json_reader::TryParseStatus::error;
        }

        // If not exhausted, we're still before '}'
        if (!frame.exhausted) {
            return json_reader::TryParseStatus::no_match;
        }

        // exhausted == true ⇒ we've advanced past the last member.
        return json_reader::TryParseStatus::ok;
    }

    // Parser: after it finishes reading the key string, it calls this
    // to switch from key → value context.
    constexpr bool consume_kv_separator(ObjectFrame& frame) {
        reset_value_string_state();

        if (!frame.obj) return true;
        if (!frame.value) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return false;
        }

        current_ = frame.value;
        return true;
    }

    // Parser: after the value is parsed, it calls this to move to the next member.
    constexpr bool consume_value_separator(ObjectFrame& frame, bool& had_comma) {
        reset_value_string_state();
        had_comma = false;

        if (!frame.obj) return true;

        // We just finished reading frame.value.
        if (!advance_object_member(frame)) {
            // No more members → we're effectively at '}'.
            current_ = frame.obj;
            had_comma = false;
        } else {
            // Moved to next key.
            current_ = frame.key;
            had_comma = true;
        }
        return true;
    }

    // ---- Skip support ----

    template<std::size_t MAX_SKIP_NESTING, class OutputSinkContainer = void>
    constexpr bool skip_json_value(OutputSinkContainer* = nullptr,
                                   std::size_t = std::numeric_limits<std::size_t>::max())
    {
        // DOM is already built; for yyjson, "skip" means "don't materialize".
        return true;
    }

    bool skip_whitespaces_till_the_end() {
        return true;
    }

private:
    yyjson_val* root_   = nullptr;
    yyjson_val* current_ = nullptr;
    ParseError  err_    = ParseError::NO_ERROR;

    void setError(ParseError e) noexcept {
        if (err_ == ParseError::NO_ERROR) err_ = e;
    }

    // --- String chunk state for *value at current_* (keys or values) ---
    const char* value_str_data_   = nullptr;
    std::size_t value_str_len_    = 0;
    std::size_t value_str_offset_ = 0;
    bool        value_str_active_ = false;

    void reset_value_string_state() noexcept {
        value_str_data_   = nullptr;
        value_str_len_    = 0;
        value_str_offset_ = 0;
        value_str_active_ = false;
    }

    // Advance object iterator to next member.
    // On success:
    //   - frame.key / frame.value updated
    //   - frame.exhausted = false
    //   - returns true
    // On end:
    //   - frame.key = frame.value = nullptr
    //   - frame.exhausted = true
    //   - returns false
    bool advance_object_member(ObjectFrame& frame) {
        if (!frame.obj) return false;

        yyjson_val* key = yyjson_obj_iter_next(&frame.it);
        if (!key) {
            frame.key       = nullptr;
            frame.value     = nullptr;
            frame.exhausted = true;
            return false;
        }

        frame.key       = key;
        frame.value     = yyjson_obj_iter_get_val(key);
        frame.exhausted = false;
        return true;
    }
};

} // namespace JsonFusion
