#include <yyjson/yyjson.h>
#include <cstring>
#include <vector>
#include <cstdint>
#include <charconv>
#include "reader_concept.hpp"

namespace JsonFusion {

class YyjsonReader {
public:
    enum class ParseError {
        NO_ERROR,
        UNEXPECTED_END_OF_DATA,
        ILLFORMED_OBJECT,
        ILLFORMED_ARRAY,
        NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE
    };
    using error_type = ParseError;

    using iterator_type = yyjson_val*; // to satisfy Parser's current() API

    // Per-container state lives here, not in the reader.
    struct ArrayFrame {
        yyjson_val*   arr      = nullptr;
        yyjson_arr_iter it{};
        std::size_t   index    = 0;     // current element index
        std::size_t   size     = 0;     // total elements
        yyjson_val*   current  = nullptr; // current element, or nullptr if empty/done
    };

    struct MapFrame {
        yyjson_val*   obj      = nullptr;
        yyjson_obj_iter it{};
        yyjson_val*   key      = nullptr; // current key node
        yyjson_val*   value    = nullptr; // current value node
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

    constexpr reader::TryParseStatus start_value_and_try_read_null() {
        if (!current_) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return reader::TryParseStatus::error;
        }
        if (!yyjson_is_null(current_)) {
            return reader::TryParseStatus::no_match;
        }
        return reader::TryParseStatus::ok;
    }

    constexpr reader::TryParseStatus read_bool(bool& b) {
        if (!current_) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return reader::TryParseStatus::error;
        }
        if (!yyjson_is_bool(current_)) {
            return reader::TryParseStatus::no_match;
        }
        b = yyjson_get_bool(current_) != 0;
        return reader::TryParseStatus::ok;
    }

    template<class NumberT>
    constexpr reader::TryParseStatus read_number(NumberT& storage) {
        if (!current_) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return reader::TryParseStatus::error;
        }
        if (!yyjson_is_num(current_)) {
            return reader::TryParseStatus::no_match;
        }


        if constexpr (std::is_integral_v<NumberT>) {
            if (yyjson_is_int(current_)) {
                auto v = yyjson_get_sint(current_);
                if (v < std::int64_t(std::numeric_limits<NumberT>::lowest()) ||
                    v > std::int64_t(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return reader::TryParseStatus::error;
                }
                storage = static_cast<NumberT>(v);
            } else if (yyjson_is_uint(current_)) {
                auto v = yyjson_get_uint(current_);
                if (v < std::uint64_t(std::numeric_limits<NumberT>::lowest()) ||
                    v > std::uint64_t(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return reader::TryParseStatus::error;
                }
                storage = static_cast<NumberT>(v);
            } else {
                auto v = yyjson_get_real(current_);
                if (v < double(std::numeric_limits<NumberT>::lowest()) ||
                    v > double(std::numeric_limits<NumberT>::max())) {
                    setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                    return reader::TryParseStatus::error;
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
                return reader::TryParseStatus::error;
            }
            storage = static_cast<NumberT>(v);
        }
        return reader::TryParseStatus::ok;

    }

    // ---- String reader (for both keys and values) ----
    //
    // Protocol:
    //  - For keys: current_ must point to the key node.
    //  - For values: current_ must point to the value node.
    //  The parser / object frame logic is responsible for setting current_.

    constexpr reader::StringChunkResult read_string_chunk(char* out, std::size_t capacity) {
        reader::StringChunkResult res{};
        res.status = reader::StringChunkStatus::error;
        res.bytes_written = 0;
        res.done = false;

        if (capacity == 0) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return res;
        }

        if (!value_str_active_) {
            if (!current_ || !yyjson_is_str(current_)) {
                res.status = reader::StringChunkStatus::no_match;
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

        res.status        = reader::StringChunkStatus::ok;
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
        if(reader::StringChunkResult r = read_string_chunk(buf, bufSize-1); !r.done || r.status != reader::StringChunkStatus::ok) {
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
    constexpr reader::IterationStatus read_array_begin(ArrayFrame& frame) {
        reset_value_string_state();

        reader::IterationStatus ret;
        if (!current_){
            setError(ParseError::ILLFORMED_ARRAY);
            ret.status = reader::TryParseStatus::error;
            return ret;
        }

        if(!yyjson_is_arr(current_)) {
            ret.status = reader::TryParseStatus::no_match;
            return ret;
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
            ret.has_value = true;
        } else {
            // Empty array – keep current_ on the array node.
            current_ = frame.arr;
            ret.has_value = false;
        }
        ret.status = reader::TryParseStatus::ok;

        return ret;
    }

    constexpr reader::IterationStatus advance_after_value(ArrayFrame& frame) {
        reset_value_string_state();
        reader::IterationStatus ret;

        if (!frame.arr) {
            setError(ParseError::ILLFORMED_ARRAY);
            ret.status = reader::TryParseStatus::error;
            return ret;
        }

        ++frame.index;
        if (frame.index < frame.size) {
            // Move to next element.
            frame.current = yyjson_arr_iter_next(&frame.it);
            current_ = frame.current;
            ret.has_value = true;
        } else {
            // Last element finished; current_ becomes the array node.
            frame.current = nullptr;
            current_ = frame.arr;
            ret.has_value = false;
        }
        ret.status = reader::TryParseStatus::ok;
        return ret;
    }


    constexpr reader::IterationStatus read_map_begin(MapFrame& frame) {
        reset_value_string_state();
        reader::IterationStatus ret;

        if (!current_){
            setError(ParseError::ILLFORMED_ARRAY);
            ret.status = reader::TryParseStatus::error;
            return ret;
        }
        if(!yyjson_is_obj(current_)) {
            ret.status = reader::TryParseStatus::no_match;
            return ret;
        }

        yyjson_val* obj = current_;
        frame.obj       = obj;
        frame.key       = nullptr;
        frame.value     = nullptr;

        yyjson_obj_iter_init(obj, &frame.it);

        // Preload first member, if any.
        if (!advance_object_member(frame)) {
            current_ = frame.obj;
            ret.has_value = false;
        } else {
            // we have a first key; set current_ to that key for key-string reading.
            current_ = frame.key;
            ret.has_value = true;
        }

        ret.status = reader::TryParseStatus::ok;
        return ret;
    }


    // Parser: after it finishes reading the key string, it calls this
    // to switch from key → value context.
    constexpr bool move_to_value(MapFrame& frame) {
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
    constexpr reader::IterationStatus advance_after_value(MapFrame& frame) {
        reset_value_string_state();

        reader::IterationStatus ret;

        if (!frame.obj) {
            setError(ParseError::ILLFORMED_OBJECT);
            ret.status = reader::TryParseStatus::error;
            return ret;
        }

        // We just finished reading frame.value.
        if (!advance_object_member(frame)) {
            // No more members → we're effectively at '}'.
            current_ = frame.obj;
            ret.has_value = false;
        } else {
            // Moved to next key.
            current_ = frame.key;
            ret.has_value = true;
        }
        ret.status = reader::TryParseStatus::ok;

        return ret;
    }

    // ---- Skip support ----

    template<std::size_t MAX_SKIP_NESTING, class OutputSinkContainer = void>
    constexpr bool skip_value(OutputSinkContainer* = nullptr,
                                   std::size_t = std::numeric_limits<std::size_t>::max())
    {
        // DOM is already built; for yyjson, "skip" means "don't materialize".
        return true;
    }

    bool finish() {
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
    //   - returns true
    // On end:
    //   - frame.key = frame.value = nullptr
    //   - returns false
    bool advance_object_member(MapFrame& frame) {
        if (!frame.obj) return false;

        yyjson_val* key = yyjson_obj_iter_next(&frame.it);
        if (!key) {
            frame.key       = nullptr;
            frame.value     = nullptr;
            return false;
        }

        frame.key       = key;
        frame.value     = yyjson_obj_iter_get_val(key);
        return true;
    }
};

} // namespace JsonFusion
