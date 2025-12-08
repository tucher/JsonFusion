#include <yyjson/yyjson.h>
#include <cstring>
#include "reader_concept.hpp"
#include <vector>
#include <cstdint>
namespace JsonFusion {
class YyjsonReader {
public:
    using iterator_type = yyjson_val*; // to satisfy Parser's current() API

    YyjsonReader(yyjson_val* root) noexcept
        :
          root_(root)
        , current_(root)
        , err_(ParseError::NO_ERROR)
    {
        // Top-level frame: treat root as "value" context.
        frames_.reserve(100);
        frames_.push_back(Frame{
            FrameKind::RootValue,
            root_,
            0,
            0
        });
    }

    // ---- Introspection ----

    iterator_type current() const noexcept {
        // For yyjson backend we don't know exact char position; use begin_.
        return current_;
    }

    ParseError getError() const noexcept { return err_; }

    // ---- Scalar readers ----

    // Your parser now calls "skip_ws_and_read_null" instead of separate ws + null.
    constexpr json_reader::TryParseStatus skip_ws_and_read_null() {
        if (!current_) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return json_reader::TryParseStatus::error;
        } else if (!yyjson_is_null(current_)) {
            return json_reader::TryParseStatus::no_match;
        }
        // Nothing else to advance here; container navigation is done by
        // consume_value_separator() / *end() calls.
        return json_reader::TryParseStatus::ok;
    }

    constexpr json_reader::TryParseStatus read_bool(bool& b) {
        if (!current_) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return json_reader::TryParseStatus::error;
        } else if (!yyjson_is_bool(current_)) {
            return json_reader::TryParseStatus::no_match;
        }
        b = yyjson_get_bool(current_) != 0;
        return json_reader::TryParseStatus::ok;
    }

    template<class NumberT, bool skipMaterializing>
    constexpr json_reader::TryParseStatus read_number(NumberT& storage) {
        if (!current_) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return json_reader::TryParseStatus::error;
        } else if (!yyjson_is_num(current_)) {
            return json_reader::TryParseStatus::no_match;
        } else {

            if constexpr (skipMaterializing) {
                // Just check it's numeric; don't actually store.
                (void)storage;
            } else {
                if constexpr (std::is_integral_v<NumberT>) {
                    // yyjson stores numbers as either i64/u64/double; we let it pick and cast.
                    // In practice you might want to detect signed/unsigned, bounds, etc.
                    if (yyjson_is_int(current_)) {
                        auto v = yyjson_get_sint(current_);
                        if(v < std::int64_t(std::numeric_limits<NumberT>::lowest()) || v > std::int64_t(std::numeric_limits<NumberT>::max())) {
                            setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                            return json_reader::TryParseStatus::error;
                        }
                        storage = static_cast<NumberT>(v);
                    } else if (yyjson_is_uint(current_)) {
                        auto v = yyjson_get_uint(current_);
                        if(v < std::uint64_t(std::numeric_limits<NumberT>::lowest()) || v > std::uint64_t(std::numeric_limits<NumberT>::max())) {
                            setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                            return json_reader::TryParseStatus::error;
                        }
                        storage = static_cast<NumberT>(v);
                    } else {
                        auto v = yyjson_get_real(current_);
                        if(v < std::int64_t(std::numeric_limits<NumberT>::lowest()) || v > std::int64_t(std::numeric_limits<NumberT>::max())) {
                            setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                            return json_reader::TryParseStatus::error;
                        }
                        storage = static_cast<NumberT>(v);
                    }
                } else {
                    static_assert(std::is_floating_point_v<NumberT>,
                                  "read_number only supports integral or floating types");

                    if (yyjson_is_real(current_)) {
                        auto v = yyjson_get_real(current_);
                        if(v < double(std::numeric_limits<NumberT>::lowest()) || v > double(std::numeric_limits<NumberT>::max())) {
                            setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                            return json_reader::TryParseStatus::error;
                        }
                        storage = static_cast<NumberT>(v);
                    } else if (yyjson_is_sint(current_)) {
                        auto v = yyjson_get_sint(current_);
                        if(v < double(std::numeric_limits<NumberT>::lowest()) || v > double(std::numeric_limits<NumberT>::max())) {
                            setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                            return json_reader::TryParseStatus::error;
                        }
                        storage = static_cast<NumberT>(v);
                    } else {
                        auto v = yyjson_get_uint(current_);
                        if(v < double(std::numeric_limits<NumberT>::lowest()) || v > double(std::numeric_limits<NumberT>::max())) {
                            setError(ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE);
                            return json_reader::TryParseStatus::error;
                        }
                        storage = static_cast<NumberT>(v);
                    }
                }
            }
            return json_reader::TryParseStatus::ok;
        }
    }

    // ---- String reader (values *and* object keys) ----

    constexpr json_reader::StringChunkResult read_string_chunk(char* out, std::size_t capacity) {
        json_reader::StringChunkResult res{};
        res.status = json_reader::StringChunkStatus::error;
        res.bytes_written = 0;
        res.done = false;

        if (capacity == 0) {
            // Can't write anything. Treat as error.
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return res;
        }

        // 1) If we have an active key string for the current object field,
        //    use that first (keys are always strings).
        if (auto* key_state = current_key_string_state()) {
            const std::size_t remaining = key_state->current_key_len - key_state->key_offset;
            const std::size_t n = remaining < capacity ? remaining : capacity;
            std::memcpy(out, key_state->current_key_data + key_state->key_offset, n);
            key_state->key_offset += n;

            res.status = json_reader::StringChunkStatus::ok;
            res.bytes_written = n;
            res.done = (key_state->key_offset >= key_state->current_key_len);
            return res;
        }else {
            if (!value_str_active_) {
                // First call for this value string
                if (!current_ || !yyjson_is_str(current_)) {
                    res.status = json_reader::StringChunkStatus::no_match;
                    return res;
                }

                value_str_data_   = yyjson_get_str(current_);
                value_str_len_    = yyjson_get_len(current_);
                value_str_offset_ = 0;
                value_str_active_ = true;
            }
            // Now we have an active value string; emit next chunk.
            const std::size_t remaining = value_str_len_ - value_str_offset_;
            const std::size_t n         = remaining < capacity ? remaining : capacity;

            std::memcpy(out, value_str_data_ + value_str_offset_, n);
            value_str_offset_ += n;

            res.status        = json_reader::StringChunkStatus::ok;
            res.bytes_written = n;
            res.done          = (value_str_offset_ >= value_str_len_);

            if (res.done) {
                // String fully consumed; next call will either no_match
                // (if current_ is not string) or start a new value.
                reset_value_string_state();
            }

            return res;
        }
    }

    // ---- Arrays ----

    constexpr bool read_array_begin() {
        if (!current_ || !yyjson_is_arr(current_)) {
            return false;
        }

        yyjson_val* arr = current_;

        Frame f{};
        f.kind  = FrameKind::Array;
        f.node  = arr;
        f.size  = yyjson_arr_size(arr);
        f.index = 0;

        yyjson_arr_iter_init(arr, &f.arr_iter);

        if (f.size > 0) {
            f.current_elem = yyjson_arr_iter_next(&f.arr_iter); // O(1)
            current_ = f.current_elem;
        } else {
            f.current_elem = nullptr;
            current_ = arr;
        }

        frames_.push_back(f);

        reset_value_string_state();
        return true;
    }

    constexpr json_reader::TryParseStatus read_array_end() {
        if (frames_.empty() || frames_.back().kind != FrameKind::Array) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return json_reader::TryParseStatus::error;
        }
        Frame& f = frames_.back();
        if (f.index < f.size) {
            // There are still unconsumed elements; "no match" == not at ']'
            return json_reader::TryParseStatus::no_match;
        }
        // All elements consumed → pop array frame, restore parent current_.
        frames_.pop_back();
        restore_current_from_parent();
        return json_reader::TryParseStatus::ok;
    }

    // ---- Objects ----

    constexpr bool read_object_begin() {
        if (!current_ || !yyjson_is_obj(current_)) {
            return false;
        }

        yyjson_val* obj = current_;

        frames_.push_back(Frame{
            FrameKind::Object,
            obj,
        });

        // Initialize first member (if any)
        if (!advance_object_member(true)) {
            // Either empty object (ok) or error (already set)
            // For empty object, member_index == size.
        }
        reset_value_string_state();
        return true;
    }

    constexpr json_reader::TryParseStatus read_object_end() {
        if (frames_.empty() || frames_.back().kind != FrameKind::Object) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return json_reader::TryParseStatus::error;
        }
        Frame& f = frames_.back();
        if (!f.obj_finished) {
            // Still members left → not at '}' yet.
            return json_reader::TryParseStatus::no_match;
        }

        // Done with object: pop frame, restore parent context.
        frames_.pop_back();
        restore_current_from_parent();
        return json_reader::TryParseStatus::ok;
    }

    // ---- Separators ----

    // In DOM mode we don't really have ':' or ','; these become "phase switches":
    //   * consume_kv_separator(): finished key → start reading value
    //   * consume_value_separator(): finished value → advance to next element/key
    constexpr bool consume_kv_separator() {
        reset_value_string_state();
        if (frames_.empty()) return true;
        Frame& f = frames_.back();
        if (f.kind != FrameKind::Object) {
            // For arrays this is never called.
            return true;
        }

        // We just finished reading the key string; now we switch current_
        // to point at the value node for this member.
        if (!f.current_value) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return false;
        }
        current_ = f.current_value;
        return true;
    }

    constexpr bool consume_value_separator(bool& had_comma) {
        reset_value_string_state();
        had_comma = false;
        if (frames_.empty()) return true;

        Frame& f = frames_.back();
        switch (f.kind) {
        case FrameKind::Array: {
            ++f.index;
            if (f.index < f.size) {
                had_comma = true;
                f.current_elem = yyjson_arr_iter_next(&f.arr_iter); // O(1)
                current_ = f.current_elem;
            } else {
                current_ = f.node;
            }
            return true;
        }

        case FrameKind::Object: {
            // We just finished parsing the value of the current member.
            // Try to advance to next member if any.
            const bool ok = advance_object_member(false);
            if (!ok) {
                // Either no more members (ok) or error (already set).
                // If no more members, had_comma stays false, current_ is object.
                if (err_ != ParseError::NO_ERROR) return false;
                had_comma = false;
            } else {
                // There *was* a next member: caller will see had_comma=true
                // and then call read_string_chunk() again for the key.
                had_comma = true;
            }
            return true;
        }

        case FrameKind::RootValue:
            // At top level there are no separators.
            return true;
        }
        return true;
    }

    // ---- Skip support (no streaming; we simply pretend it's skipped) ----

    template<std::size_t MAX_SKIP_NESTING, class OutputSinkContainer = void>
    constexpr bool skip_json_value(OutputSinkContainer* /*outputContainer*/ = nullptr,
                                   std::size_t /*MaxStringLength*/ = std::numeric_limits<std::size_t>::max())
    {
        // For a DOM backend, "skipping" is effectively "do nothing":
        // the DOM is already built, and parser won't materialize into the user model.
        // We just claim success.
        return true;
    }

    bool skip_whitespaces_till_the_end() {
        return true;
    }
private:
    enum class FrameKind { RootValue, Array, Object };

    struct Frame {
        FrameKind   kind;
        yyjson_val* node;          // array or object node (or root)
        std::size_t index = 0;
        std::size_t size  = 0;
        yyjson_arr_iter arr_iter{};
        yyjson_val* current_elem = nullptr;
        bool obj_finished = false;
        // For objects only:
        // we cache current key + value for member at (index-1) or index.
        const char* current_key_data = nullptr;
        std::size_t current_key_len  = 0;
        std::size_t key_offset       = 0;   // for chunking
        yyjson_val* current_value    = nullptr;

        yyjson_obj_iter iter;

    };

    yyjson_val*  root_;

    std::vector<Frame> frames_;
    yyjson_val*        current_ = nullptr;  // "current scalar / container" in play
    ParseError         err_;

    void setError(ParseError e) noexcept {
        if (err_ == ParseError::NO_ERROR) err_ = e;
    }

    // Helper: get StringState for current key (if in object).
    Frame* current_key_string_state() {
        if (frames_.empty()) return nullptr;
        Frame& f = frames_.back();
        if (f.kind != FrameKind::Object) return nullptr;
        if (!f.current_key_data) return nullptr;
        if (f.key_offset >= f.current_key_len) return nullptr;
        return &f;
    }

    void restore_current_from_parent() {
        reset_value_string_state();
        if (frames_.empty()) {
            current_ = root_;
            return;
        }
        Frame& parent = frames_.back();
        switch (parent.kind) {
        case FrameKind::RootValue:
            current_ = parent.node;
            break;
        case FrameKind::Array:
        case FrameKind::Object:
            // When container is active, "current_" for end-of-container
            // is the container node itself.
            current_ = parent.node;
            break;
        }
    }

    /// Advance object member cursor.
    /// If `initial` is true, we are called from read_object_begin() to
    /// set up the first member. Otherwise we are called from
    /// consume_value_separator() to move to the next member.
    ///
    /// Returns:
    ///   - true  : there *is* a next member; key/value cached in frame
    ///   - false : no more members; frame.index == frame.size
    ///             (err_ left untouched)
    bool advance_object_member(bool initial) {
        if (frames_.empty() || frames_.back().kind != FrameKind::Object)
            return false;

        Frame& f = frames_.back();
        yyjson_val* obj = f.node;

        if (initial) {
            yyjson_obj_iter_init(obj, &f.iter);
        }

        yyjson_val* key = yyjson_obj_iter_next(&f.iter);
        if (!key) {
            // No more members.
            f.current_key_data = nullptr;
            f.current_key_len  = 0;
            f.key_offset       = 0;
            f.current_value    = nullptr;
            f.obj_finished = true;
            return false;
        }

        f.current_key_data = yyjson_get_str(key);
        f.current_key_len  = yyjson_get_len(key);
        f.key_offset       = 0;
        f.current_value    =  yyjson_obj_iter_get_val(key);


        return true;
    }

    // --- state for value string chunking (non-key) ---
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
};

}
