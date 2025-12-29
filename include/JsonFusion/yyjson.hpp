#pragma once
#include <yyjson.h>
#include <cstring>
#include <vector>
#include <cstdint>
#include <charconv>
#include "reader_concept.hpp"
#include "writer_concept.hpp"

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

    reader::TryParseStatus start_value_and_try_read_null() {
        if (!current_) {
            setError(ParseError::UNEXPECTED_END_OF_DATA);
            return reader::TryParseStatus::error;
        }
        if (!yyjson_is_null(current_)) {
            return reader::TryParseStatus::no_match;
        }
        return reader::TryParseStatus::ok;
    }

    reader::TryParseStatus read_bool(bool& b) {
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
    reader::TryParseStatus read_number(NumberT& storage) {
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

    reader::StringChunkResult read_string_chunk(char* out, std::size_t capacity) {
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

    bool read_key_as_index(std::size_t & out) {
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
    reader::IterationStatus read_array_begin(ArrayFrame& frame) {
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

    reader::IterationStatus advance_after_value(ArrayFrame& frame) {
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


    reader::IterationStatus read_map_begin(MapFrame& frame) {
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
    bool move_to_value(MapFrame& frame) {
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
    reader::IterationStatus advance_after_value(MapFrame& frame) {
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
    bool skip_value(OutputSinkContainer* = nullptr,
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

static_assert(reader::ReaderLike<YyjsonReader>);


class YyjsonWriter {
public:
    using iterator_type = yyjson_mut_val*;

    enum class Error {
        None,
        AllocFailed,
        InvalidState
    };

    using error_type = Error;

    struct ArrayFrame {
        yyjson_mut_val* node = nullptr;

        // link to parent "scope" (if any)
        void* parent_frame   = nullptr;
        bool  parent_is_map  = false;
    };

    struct MapFrame {
        yyjson_mut_val* node = nullptr;

        // parent scope
        void* parent_frame   = nullptr;
        bool  parent_is_map  = false;

        // key state
        bool        expecting_key = true;
        bool        use_index_key = false;
        std::size_t pending_index = 0;
        std::string pending_key;
    };

    explicit YyjsonWriter(yyjson_mut_doc * doc)
        : doc_(doc)
        , root_(nullptr)
        , current_(nullptr)
        , error_(Error::None)
        , scope_kind_(ScopeKind::Root)
        , scope_frame_(nullptr)
    {
        if (!doc_) {
            error_ = Error::AllocFailed;
        }
    }

    // ========== required API for WriterLike ==========

    iterator_type& current() noexcept {
        return current_;
    }

    error_type getError() const noexcept {
        return error_;
    }

    // ---- containers ----

    bool write_array_begin(std::size_t const& /*size*/, ArrayFrame& frame) {
        if (!ensure_ok()) return false;
        yyjson_mut_val* arr = yyjson_mut_arr(doc_);
        if (!arr) return fail(Error::AllocFailed);

        // Attach array as a value of current scope
        if (!attach_value_to_current(arr)) {
            return false;
        }

        // Fill frame and switch scope to this array
        frame.node         = arr;
        frame.parent_frame = scope_frame_;
        frame.parent_is_map = (scope_kind_ == ScopeKind::Map);

        scope_kind_  = ScopeKind::Array;
        scope_frame_ = &frame;

        current_ = arr;
        return true;
    }

    bool write_map_begin(std::size_t const& /*size*/, MapFrame& frame) {
        if (!ensure_ok()) return false;

        yyjson_mut_val* obj = yyjson_mut_obj(doc_);
        if (!obj) return fail(Error::AllocFailed);

        // Attach map as a value of current scope
        if (!attach_value_to_current(obj)) {
            return false;
        }

        frame.node          = obj;
        frame.parent_frame  = scope_frame_;
        frame.parent_is_map = (scope_kind_ == ScopeKind::Map);

        frame.expecting_key = true;
        frame.use_index_key = false;
        frame.pending_index = 0;
        frame.pending_key.clear();

        scope_kind_  = ScopeKind::Map;
        scope_frame_ = &frame;

        current_ = obj;
        return true;
    }

    // “Separator” hook: called *between* elements.
    // For DOM / yyjson this is a no-op; commas are implicit in tree.
    bool advance_after_value(ArrayFrame& ) {
        return ensure_ok();
    }

    bool advance_after_value(MapFrame& ) {
        return ensure_ok();
    }

    // For textual JSON this would emit ':'. For yyjson we just sanity-check.
    bool move_to_value(MapFrame& frame) {
        if (!ensure_ok()) return false;
        if (scope_kind_ != ScopeKind::Map || scope_frame_ != &frame) {
            return fail(Error::InvalidState);
        }

        if (frame.expecting_key) {
            // value without key
            return fail(Error::InvalidState);
        }
        // nothing to do; next write_* will attach as value for pending key
        return true;
    }

    // String keys are handled by write_string() when in “key mode”.
    // write_key_as_index is for indexes_as_keys.
    bool write_key_as_index(std::size_t const& idx) {
        if (!ensure_ok()) return false;
        if (scope_kind_ != ScopeKind::Map) return fail(Error::InvalidState);

        MapFrame* frame = static_cast<MapFrame*>(scope_frame_);
        if (!frame->expecting_key) {
            return fail(Error::InvalidState);
        }

        frame->use_index_key  = true;
        frame->pending_index  = idx;
        frame->pending_key.clear();
        frame->expecting_key  = false;
        return true;
    }

    bool write_array_end(ArrayFrame& frame) {
        if (!ensure_ok()) return false;
        if (scope_kind_ != ScopeKind::Array || scope_frame_ != &frame) {
            return fail(Error::InvalidState);
        }

        // restore parent scope from frame
        restore_parent_scope(frame.parent_frame, frame.parent_is_map);
        return true;
    }

    bool write_map_end(MapFrame& frame) {
        if (!ensure_ok()) return false;
        if (scope_kind_ != ScopeKind::Map || scope_frame_ != &frame) {
            return fail(Error::InvalidState);
        }

        if (!frame.expecting_key) {
            // have a key without a value
            return fail(Error::InvalidState);
        }

        restore_parent_scope(frame.parent_frame, frame.parent_is_map);
        return true;
    }

    // ---- primitives ----

    bool write_null() {
        if (!ensure_ok()) return false;
        yyjson_mut_val* v = yyjson_mut_null(doc_);
        if (!v) return fail(Error::AllocFailed);
        return attach_value_to_current(v);
    }

    bool write_bool(bool const& b) {
        if (!ensure_ok()) return false;
        yyjson_mut_val* v = yyjson_mut_bool(doc_, b);
        if (!v) return fail(Error::AllocFailed);
        return attach_value_to_current(v);
    }

    template<class NumberT>
    bool write_number(NumberT const& value) {
        if (!ensure_ok()) return false;

        yyjson_mut_val* v = nullptr;
        if constexpr (std::is_floating_point_v<NumberT>) {
            v = yyjson_mut_real(doc_, static_cast<double>(value));
        } else if constexpr (std::is_signed_v<NumberT>) {
            v = yyjson_mut_sint(doc_, static_cast<int64_t>(value));
        } else if constexpr (std::is_unsigned_v<NumberT>) {
            v = yyjson_mut_uint(doc_, static_cast<uint64_t>(value));
        } else {
            static_assert(std::is_arithmetic_v<NumberT>,
                          "write_number only supports arithmetic types");
        }

        if (!v) return fail(Error::AllocFailed);
        return attach_value_to_current(v);
    }

    // String writing:
    //  - In map & expecting_key → record key in MapFrame
    //  - Else → create value string node and attach
    bool write_string(char const* data, std::size_t size, bool null_terminated = false) {
        if (!ensure_ok()) return false;

        if (null_terminated) {
            size = std::strlen(data);
        }

        if (scope_kind_ == ScopeKind::Map) {
            MapFrame* frame = static_cast<MapFrame*>(scope_frame_);
            if (frame->expecting_key) {
                frame->pending_key.assign(data, size);
                frame->use_index_key  = false;
                frame->pending_index  = 0;
                frame->expecting_key  = false;
                return true;
            }
        }

        yyjson_mut_val* v = yyjson_mut_strncpy(doc_, data, size);
        if (!v) return fail(Error::AllocFailed);
        return attach_value_to_current(v);
    }

    // ---- finish ----

    bool finish() {
        if (!ensure_ok()) return false;
        if (!doc_) return fail(Error::InvalidState);

        if (!root_) {
            yyjson_mut_val* v = yyjson_mut_null(doc_);
            if (!v) return fail(Error::AllocFailed);
            yyjson_mut_doc_set_root(doc_, v);
            root_ = v;
        }

        // You *could* assert here that scope_kind_ == Root, but
        // serializer already guarantees balanced begin/end.
        return true;
    }


private:
    enum class ScopeKind { Root, Array, Map };

    yyjson_mut_doc* doc_;
    yyjson_mut_val* root_;
    yyjson_mut_val* current_;
    error_type      error_;

    ScopeKind scope_kind_;
    void*     scope_frame_;

    bool ensure_ok() const noexcept {
        return error_ == Error::None;
    }

    bool fail(Error e) {
        if (error_ == Error::None) {
            error_ = e;
        }
        return false;
    }

    void restore_parent_scope(void* parent_frame, bool parent_is_map) {
        if (!parent_frame) {
            scope_kind_  = ScopeKind::Root;
            scope_frame_ = nullptr;
        } else {
            scope_kind_  = parent_is_map ? ScopeKind::Map : ScopeKind::Array;
            scope_frame_ = parent_frame;
        }
    }

    bool attach_value_to_current(yyjson_mut_val* v) {
        if (!ensure_ok()) return false;

        switch (scope_kind_) {
        case ScopeKind::Root:
            if (root_) {
                return fail(Error::InvalidState); // multiple roots
            }
            root_ = v;
            yyjson_mut_doc_set_root(doc_, v);
            break;

        case ScopeKind::Array: {
            auto* frame = static_cast<ArrayFrame*>(scope_frame_);
            if (!frame || !frame->node) return fail(Error::InvalidState);
            if (!yyjson_mut_arr_add_val(frame->node, v)) {
                return fail(Error::AllocFailed);
            }
            break;
        }

        case ScopeKind::Map: {
            auto* frame = static_cast<MapFrame*>(scope_frame_);
            if (!frame || !frame->node) return fail(Error::InvalidState);
            if (frame->expecting_key) {
                // got a value while still waiting for a key
                return fail(Error::InvalidState);
            }

            yyjson_mut_val* key_node = nullptr;
            if (frame->use_index_key) {
                char buf[32];
                auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), frame->pending_index);
                if (ec != std::errc()) {
                    return fail(Error::InvalidState);
                }
                std::size_t len = static_cast<std::size_t>(ptr - buf);
                key_node = yyjson_mut_strncpy(doc_, buf, len);
                frame->use_index_key = false;
                frame->pending_index = 0;
            } else {
                key_node = yyjson_mut_strncpy(doc_,
                                              frame->pending_key.data(),
                                              frame->pending_key.size());
                frame->pending_key.clear();
            }

            if (!key_node) return fail(Error::AllocFailed);
            if (!yyjson_mut_obj_add(frame->node, key_node, v)) {
                return fail(Error::AllocFailed);
            }
            frame->expecting_key = true;
            break;
        }
        }

        current_ = v;
        return true;
    }
};
static_assert(writer::WriterLike<YyjsonWriter>);
} // namespace JsonFusion
