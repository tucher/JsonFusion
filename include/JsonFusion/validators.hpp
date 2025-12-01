#pragma once
#include <cstdint>
#include <type_traits>
#include <limits>
#include <utility>
#include <bitset>

#include "options.hpp"
#include "struct_introspection.hpp"
#include "static_schema.hpp"
#include <iostream>
#include <map>
namespace JsonFusion {

namespace validators {

// ============================================================================
// Generic Name Search Utilities (Extracted from Parser)
// ============================================================================

/// Generic descriptor for named entities (fields, keys, etc.)
struct NameDescr {
    std::string_view name;
    std::size_t originalIndex;
};

// Thresholds for selecting search strategy at compile time
inline constexpr std::size_t MaxNamesCountForLinearSearch = 8;
inline constexpr std::size_t MaxNameLengthForLinearSearch = 32;

/// Binary search strategy: incrementally narrows candidate range per character
struct IncrementalBinaryNameSearch {
    using It = const NameDescr*;

    It first;         // current candidate range [first, last)
    It last;
    It original_begin; // original begin, for reset
    It original_end;  // original end, used as "empty result"
    std::size_t depth = 0; // how many characters have been fed

    constexpr IncrementalBinaryNameSearch(It begin, It end)
        : first(begin), last(end), original_begin(begin), original_end(end), depth(0) {}

    // Feed next character; narrows [first, last) by character at position `depth`.
    // Returns true if any candidates remain after this step.
    constexpr bool step(char ch) {
        if (first == last)
            return false;

        // projection: NameDescr -> char at current depth
        auto char_at_depth = [this](const NameDescr& f) -> char {
            std::string_view s = f.name;
            // sentinel: '\0' means "past the end"
            return depth < s.size() ? s[depth] : '\0';
        };

        // lower_bound: first element with char_at_depth(elem) >= ch
        auto lower = std::ranges::lower_bound(
            first, last, ch, {}, char_at_depth
        );

        // upper_bound: first element with char_at_depth(elem) > ch
        auto upper = std::ranges::upper_bound(
            lower, last, ch, {}, char_at_depth
        );

        first = lower;
        last  = upper;
        ++depth;
        return first != last;
    }

    // Return:
    //   - pointer to unique NameDescr if exactly one matches AND
    //     fully typed (depth >= name.size())
    //   - original_end if 0 matches OR >1 matches OR undertyped.
    constexpr It result() const {
        if (first == last)
            return original_end; // 0 matches

        // exactly one candidate
        const NameDescr& candidate = *first;

        // undertyping: not all characters of the name have been given
        if (depth != candidate.name.size())
            return original_end;

        return first;
    }
    
    // Reset for next key
    constexpr void reset() {
        first = original_begin;
        last = original_end;
        depth = 0;
    }
};

/// Linear search strategy: buffers the name, does simple linear search at the end
template<std::size_t MaxLen>
struct BufferedLinearNameSearch {
    using It = const NameDescr*;

    It begin;         // name descriptors range [begin, end)
    It end;
    char buffer[MaxLen];
    std::size_t length = 0;
    bool overflown = false;

    constexpr BufferedLinearNameSearch(It begin_, It end_)
        : begin(begin_), end(end_), buffer{}, length(0) {}

    // Simply buffer the character, no searching yet
    constexpr bool step(char ch) {
        if (length < MaxLen) {
            buffer[length++] = ch;
            return true;
        }
        // Name too long, won't match anything
        overflown = true;
        return false;
    }

    // Perform linear search through all names
    constexpr It result() const {
        if(overflown) return end;
        std::string_view key(buffer, length);
        for (It it = begin; it != end; ++it) {
            if (it->name == key) {
                return it;
            }
        }
        return end; // not found
    }
    
    // Reset for next key
    constexpr void reset() {
        for (std::size_t i = 0; i < MaxLen; ++i) {
            buffer[i] = '\0';
        }
        length = 0;
        overflown = false;
    }
};

/// Adapter that selects strategy at compile time based on name count and max length
template<std::size_t NameCount, std::size_t MaxNameLen>
struct AdaptiveNameSearch {
    using It = const NameDescr*;
    
    // static constexpr bool useLinear =
    //     (NameCount <= MaxNamesCountForLinearSearch) &&
    //     (MaxNameLen <= MaxNameLengthForLinearSearch);

    static constexpr bool useLinear = true;

    using SearchImpl = std::conditional_t<
        useLinear,
        BufferedLinearNameSearch<MaxNameLen>,
        IncrementalBinaryNameSearch
    >;

    SearchImpl impl;

    constexpr AdaptiveNameSearch(It begin, It end)
        : impl(begin, end) {}

    constexpr bool step(char ch) {
        return impl.step(ch);
    }

    constexpr It result() const {
        return impl.result();
    }
    
    constexpr void reset() {
        impl.reset();
    }
};

/// Helper to compute sorted keys and metadata for key set validators
template<ConstString... Keys>
struct KeySetHelper {
    static constexpr std::size_t keyCount = sizeof...(Keys);
    
    // Build sorted array of key names at compile time
    static constexpr std::array<NameDescr, keyCount> sortedKeys = []() consteval {
        std::array<NameDescr, keyCount> arr;
        std::size_t idx = 0;
        ((arr[idx++] = NameDescr{Keys.toStringView(), idx}), ...);
        std::ranges::sort(arr, {}, &NameDescr::name);
        return arr;
    }();
    
    static constexpr std::size_t maxKeyLength = []() consteval {
        std::size_t maxLen = 0;
        for (const auto& k : sortedKeys) {
            if (k.name.size() > maxLen) maxLen = k.name.size();
        }
        return maxLen;
    }();
};

// ============================================================================
// Schema Errors
// ============================================================================

enum class SchemaError  {
    none                     ,
    number_out_of_range              ,
    string_length_out_of_range       ,
    array_items_count_out_of_range   ,
    missing_required_fields          ,
    map_properties_count_out_of_range,
    map_key_length_out_of_range      ,
    wrong_constant_value             ,
    map_key_not_allowed              ,
    map_key_forbidden                ,
    map_missing_required_key
};



struct ValidationResult {
    SchemaError  m_error  = SchemaError::none;
    std::size_t validatorIndex;
    constexpr operator bool() const {
        return m_error == SchemaError::none;
    }

    constexpr SchemaError error() const {
        return m_error;
    }
    constexpr std::size_t validator_index() const {
        return validatorIndex;
    }
};


namespace detail {



struct ValidationCtx {
    SchemaError  m_error  = SchemaError::none;
    std::size_t validatorIndex = std::numeric_limits<std::size_t>::max();
    constexpr void setError(SchemaError e, std::size_t validatorOptIndex) {
        m_error = e;
        validatorIndex = validatorOptIndex;
    }
    constexpr ValidationResult result() const {
        return ValidationResult{m_error, validatorIndex};
    }
};

struct empty_state {};

template<class Opt, class Storage, class = void>
struct option_state_type {
    using type = empty_state;
};

template<class Opt, class Storage>
struct option_state_type<Opt, Storage, std::void_t<typename Opt::template state<Storage>>> {
    using type = typename Opt::template state<Storage>;
};

template<class Opt, class Storage>
using option_state_t = typename option_state_type<Opt, Storage>::type;


template <class FieldOpts, class Storage>
struct validator_state  {

};



template<class T, class... Opts, class Storage>
struct validator_state<options::detail::field_options<T, Opts...>, Storage> {
    static constexpr std::size_t OptsCount = sizeof...(Opts);
    using OptsTuple   = std::tuple<Opts...>;
    using StatesTuple = std::tuple<detail::option_state_t<Opts, Storage>...>;
    StatesTuple states{};
    template<class Tag, class... Args>
    constexpr bool validate(const Storage& storage, ValidationCtx& ctx, const Args&... args) {
        return validate_impl<Tag>(
            std::make_index_sequence<OptsCount>{},
            storage, ctx, args...
        );

        // bool ok = true;
        // std::apply(
        //     [&](auto&... st) {
        //         // iterate over options, in lockstep with states
        //         ((ok = ok && call_one<Tag, Opts>(st, storage, ctx, args...)), ...);
        //     },
        //     states
        //     );
        // return ok;
    }

private:
    template<class Tag, class... Args, std::size_t... I>
    constexpr bool validate_impl(std::index_sequence<I...>, const Storage& storage,
                                 ValidationCtx& ctx, const Args&... args)
    {
        bool ok = true;
        // fold over indices I...
        ( (ok = ok && validate_one<Tag, I>(storage, ctx, args...)), ... );
        return ok;
    }

    template<class Tag, std::size_t I, class... Args>
    constexpr bool validate_one(const Storage& storage, ValidationCtx& ctx, const Args&... args) {
        using Opt   = std::tuple_element_t<I, OptsTuple>;
        using State = std::tuple_element_t<I, StatesTuple>;

        auto& st = std::get<I>(states);
        return call_one<Tag, I, Opt>(st, storage, ctx, args...);
    }

    template<class Tag, std::size_t Index, class Opt, class State, class... Args>
    static constexpr bool call_one(
        State&        st,
        const Storage& storage,
        ValidationCtx&          ctx,
        const Args&... args
        )
    {
        if constexpr (!std::is_same_v<State, empty_state>) {
            if constexpr (requires { Opt::template validate<Tag, Index>(st, storage, ctx, args...); }) {
                return Opt::template validate<Tag, Index>(st, storage, ctx, args...);
            } else {
                return true;
            }

        } else {

            // Stateless option
            if constexpr (requires { Opt::template validate<Tag, Index>(storage, ctx, args...); }) {
                return Opt::template validate<Tag, Index>(storage, ctx, args...);
            } else if constexpr (requires { Opt::validate(storage, ctx, args...); }) {
                return Opt::validate(storage, ctx, args...);
            } else {
                return true;
            }

        }
    }
};

template<class Storage>
struct validator_state<options::detail::no_options, Storage> {
    template<class Tag, class Ctx, class... Args>
    constexpr bool validate(const Storage&, Ctx&, const Args&...) {
        return true;
    }
};





namespace parsing_events_tags {

struct bool_parsing_finished{};
struct number_parsing_finished{};
struct string_parsed_some_chars{};
struct string_parsing_finished{};
struct array_item_parsed{};
struct array_parsing_finished{};
struct object_parsing_finished{};
struct descrtuctured_object_parsing_finished{};
struct map_key_parsed_some_chars{};
struct map_key_finished{};
struct map_value_parsed{};
struct map_entry_parsed{};
struct map_parsing_finished{};

}
} //namespace detail


template<auto C>
struct constant {
    static_assert(!is_const_string<decltype(C)>::value, "Use string_constant instead");
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, detail::parsing_events_tags::bool_parsing_finished>
    static constexpr bool validate(const Storage&  v, detail::ValidationCtx&  ctx) {
        static_assert(std::is_same_v<decltype(C), bool>, "Constant is not bool");
        if(v != C) {
            ctx.setError(SchemaError::wrong_constant_value, Index);
            return false;
        } else {
            return true;
        }
    }
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, detail::parsing_events_tags::number_parsing_finished>
    static constexpr bool validate(const Storage&  v, detail::ValidationCtx&  ctx) {
        static_assert(static_schema::JsonNumber<decltype(C)>, "Constant is not number");
        if(v != C) {
            ctx.setError(SchemaError::wrong_constant_value, Index);
            return false;
        } else {
            return true;
        }
    }

    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, detail::parsing_events_tags::string_parsing_finished>
    static constexpr bool validate(const Storage&  v, detail::ValidationCtx&  ctx, std::size_t size, const std::string_view & value) {
        static_assert(is_const_string<decltype(C)>::value, "Constant is not string");
        if(value != C.toStringView()) {
            ctx.setError(SchemaError::wrong_constant_value, Index);
            return false;
        } else {
            return true;
        }
    }
    static constexpr std::string_view to_string() {
        return "constant does not match";
    }
};

template<ConstString C>
struct string_constant {
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, detail::parsing_events_tags::string_parsing_finished>
    static constexpr bool validate(const Storage&  v, detail::ValidationCtx&  ctx, std::size_t size, const std::string_view & value) {
        static_assert(is_const_string<decltype(C)>::value, "Constant is not string");
        if(value != C.toStringView()) {
            ctx.setError(SchemaError::wrong_constant_value, Index);
            return false;
        } else {
            return true;
        }
    }
    static constexpr std::string_view to_string() {
        return "constant does not match";
    }
};

// ============================================================================
// String Enum Validation
// ============================================================================

template<ConstString... Values>
struct enum_values {    
    using ValueSet = KeySetHelper<Values...>;
    static constexpr std::size_t valueCount = ValueSet::keyCount;
    static constexpr auto& sortedValues = ValueSet::sortedKeys;
    static constexpr std::size_t maxValueLength = ValueSet::maxKeyLength;
    
    template<class Storage>
    struct state {
        AdaptiveNameSearch<valueCount, maxValueLength> searcher{
            sortedValues.data(), 
            sortedValues.data() + sortedValues.size()
        };
    };
    
    // Incremental validation during string parsing (per-character)
    template<class Tag, std::size_t Index, class Storage>
    static constexpr bool validate(state<Storage>& st, const Storage& str,
                                   detail::ValidationCtx& ctx, char c)
        requires std::is_same_v<Tag, detail::parsing_events_tags::string_parsed_some_chars>
    {
        // Feed character to incremental search
        if (!st.searcher.step(c)) {
            // Early rejection: no enum value matches this prefix
            ctx.setError(SchemaError::wrong_constant_value, Index);
            st.searcher.reset();
            return false;
        }
        return true;
    }
    
    // Final validation after string is fully parsed
    template<class Tag, std::size_t Index, class Storage>
    static constexpr bool validate(state<Storage>& st, const Storage& str,
                                   detail::ValidationCtx& ctx, std::size_t parsed_size, const std::string_view & value)
        requires std::is_same_v<Tag, detail::parsing_events_tags::string_parsing_finished>
    {
        auto result = st.searcher.result();
        st.searcher.reset();
        
        // Check if we have a valid match
        if (result == sortedValues.end()) {
            // String does not match any enum value (or is empty)
            ctx.setError(SchemaError::wrong_constant_value, Index);
            return false;
        }
        
        // Verify the parsed string length matches the enum value length
        // (This handles cases where the incremental validation passed but lengths don't match)
        if (parsed_size != result->name.size()) {
            ctx.setError(SchemaError::wrong_constant_value, Index);
            return false;
        }
        
        return true;
    }

    static constexpr std::string_view to_string() {
        return "string value is not in enum list";
    }
};

template<auto Min, auto Max>
struct range {
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, detail::parsing_events_tags::number_parsing_finished>
    static constexpr bool validate(const Storage& val, detail::ValidationCtx&ctx) {
        if constexpr (std::is_floating_point_v<Storage>){
            static_assert(
                std::numeric_limits<Storage>::lowest() <= Min && Min <= std::numeric_limits<Storage>::max() &&
                std::numeric_limits<Storage>::lowest() <= Max && Max <= std::numeric_limits<Storage>::max(),
                "[[[ JsonFusion ]]] range: Provided min or max values are outside of field's storage type range");
        } else if constexpr(std::is_integral_v<Storage>) {
            static_assert(std::in_range<Storage>(Min) && std::in_range<Storage>(Max),
                "[[[ JsonFusion ]]] range: Provided min or max values are outside of field's storage type range");
        }
        else {
            static_assert(!sizeof(Storage), "[[[ JsonFusion ]]] Option is not applicable to field");
        }
        if (val < Min || val > Max) {
            ctx.setError(SchemaError::number_out_of_range, Index);
            return false;
        } else {
            return true;
        }
    }

    static constexpr std::string_view to_string() {
        return "number value is outside of range";
    }
};


template<std::size_t N>
struct min_length {
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, detail::parsing_events_tags::string_parsing_finished>
    static constexpr bool validate(const Storage& val, detail::ValidationCtx&ctx, std::size_t size, const std::string_view & value) {
        if(size >= N) {
            return true;
        } else {
            ctx.setError(SchemaError::string_length_out_of_range, Index);
            return false;
        }
    }
    static constexpr std::string_view to_string() {
        return "string length is less than required";
    }
};

template<std::size_t N>
struct max_length {
    template<class Storage>
    struct state {
        std::size_t char_count = 0;
    };

    template<class Tag, std::size_t Index, class Storage>
    static constexpr bool validate(state<Storage>& st, const Storage& val,
                                   detail::ValidationCtx& ctx, char c)
        requires std::is_same_v<Tag, detail::parsing_events_tags::string_parsed_some_chars>
    {
        st.char_count++;
        if (st.char_count > N) {
            // Early rejection: string is too long
            ctx.setError(SchemaError::string_length_out_of_range, Index);
            return false;
        }
        return true;
    }
    static constexpr std::string_view to_string() {
        return "string length is greater than required";
    }
};

template<std::size_t N>
struct min_items {
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, detail::parsing_events_tags::array_parsing_finished>
    static constexpr bool validate(const Storage& val, detail::ValidationCtx&ctx, std::size_t count) {
        if(count >= N) {
            return true;
        } else {
            ctx.setError(SchemaError::array_items_count_out_of_range, Index);
            return false;
        }
    }
    static constexpr std::string_view to_string() {
        return "too few elements in array";
    }
};

template<std::size_t N>
struct max_items {
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, detail::parsing_events_tags::array_item_parsed>
    static constexpr bool validate(const Storage& val, detail::ValidationCtx&ctx, std::size_t count) {
        if(count <= N) {
            return true;
        } else {
            ctx.setError(SchemaError::array_items_count_out_of_range, Index);
            return false;
        }
    }
    static constexpr std::string_view to_string() {
        return "too many elements in array";
    }
};

template <ConstString ... NotRequiredNames>
struct not_required {
    template<class Tag, std::size_t Index, class Storage, class FH>
        requires std::is_same_v<Tag, detail::parsing_events_tags::object_parsing_finished>
    static constexpr bool validate(const Storage& val, detail::ValidationCtx&ctx, const std::bitset<FH::fieldsCount> & seen, const FH&) {
        static_assert(
            ((FH::indexInSortedByName(NotRequiredNames.toStringView()) != -1) &&...),
            "Fields in 'not_required' are not presented in json model of object, check c++ fields names or 'key' annotations");

        // Builds the required fields mask at compile time: requiredMask[i] = true if field i is not in not_required list
        // This is computed  per template instantiation, not at runtime
        constexpr auto requiredMask = []() consteval {
            std::bitset<FH::fieldsCount> mask{};
            mask.set(); // all required by default
            // Mark these as not-required
            ((mask.reset(FH::indexInSortedByName(NotRequiredNames.toStringView()))), ...);
            return mask;
        }();

        if ((seen & requiredMask) != requiredMask) {
            ctx.setError(SchemaError::missing_required_fields, Index);
            return false;
        }
        return true;
    }
    static constexpr std::string_view to_string() {
        return "missing field in struct ,not marked as not-requried";
    }
};

// ============================================================================
// Map/Object Property Count Validators
// ============================================================================

template<std::size_t N>
struct min_properties {
    static constexpr std::size_t value = N;
    
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, detail::parsing_events_tags::map_parsing_finished>
    static constexpr bool validate(const Storage& val, detail::ValidationCtx& ctx, std::size_t count) {
        if (count >= N) {
            return true;
        } else {
            ctx.setError(SchemaError::map_properties_count_out_of_range, Index);
            return false;
        }
    }
    static constexpr std::string_view to_string() {
        return "too few kv pairs in map";
    }
};

template<std::size_t N>
struct max_properties {
    static constexpr std::size_t value = N;
    
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, detail::parsing_events_tags::map_entry_parsed>
    static constexpr bool validate(const Storage& val, detail::ValidationCtx& ctx, std::size_t count) {
        if (count <= N) {
            return true;
        } else {
            ctx.setError(SchemaError::map_properties_count_out_of_range, Index);
            return false;
        }
    }
    static constexpr std::string_view to_string() {
        return "too many kv pairs in map";
    }
};

// ============================================================================
// Map Key Validation
// ============================================================================

template<std::size_t N>
struct min_key_length {
    static constexpr std::size_t value = N;
    
    template<class Tag, std::size_t Index, class Storage, class KeyType>
        requires std::is_same_v<Tag, detail::parsing_events_tags::map_key_finished>
    static constexpr bool validate(const Storage& val, detail::ValidationCtx& ctx, const KeyType& key, std::size_t entry_index) {
        // Calculate key length (up to null terminator for char arrays)
        std::size_t key_len = 0;
        if constexpr (requires { key.size(); key[0]; }) {
            for (std::size_t i = 0; i < key.size(); ++i) {
                if (key[i] == '\0') break;
                key_len++;
            }
        }
        
        if (key_len >= N) {
            return true;
        } else {
            ctx.setError(SchemaError::map_key_length_out_of_range, Index);
            return false;
        }
    }
    static constexpr std::string_view to_string() {
        return "map key length is too short";
    }
};

template<std::size_t N>
struct max_key_length {
    static constexpr std::size_t value = N;
    
    template<class Tag, std::size_t Index, class Storage, class KeyType>
        requires std::is_same_v<Tag, detail::parsing_events_tags::map_key_finished>
    static constexpr bool validate(const Storage& val, detail::ValidationCtx& ctx, const KeyType& key, std::size_t entry_index) {
        // Calculate key length (up to null terminator for char arrays)
        std::size_t key_len = 0;
        if constexpr (requires { key.size(); key[0]; }) {
            for (std::size_t i = 0; i < key.size(); ++i) {
                if (key[i] == '\0') break;
                key_len++;
            }
        }
        
        if (key_len <= N) {
            return true;
        } else {
            ctx.setError(SchemaError::map_key_length_out_of_range, Index);
            return false;
        }
    }
    static constexpr std::string_view to_string() {
        return "map key length is too long";
    }
};

template<ConstString... Keys>
struct required_keys {
    
    using KeySet = KeySetHelper<Keys...>;
    static constexpr std::size_t keyCount = KeySet::keyCount;
    static constexpr auto& sortedKeys = KeySet::sortedKeys;
    static constexpr std::size_t maxKeyLength = KeySet::maxKeyLength;
    
    template<class Storage>
    struct state {
        std::bitset<keyCount> seen{};  // Track which required keys were parsed
        AdaptiveNameSearch<keyCount, maxKeyLength> searcher{
            sortedKeys.data(), 
            sortedKeys.data() + sortedKeys.size()
        };
    };
    
    // Incremental validation during key parsing (per-character)
    template<class Tag, std::size_t Index, class Storage>
    static constexpr bool validate(state<Storage>& st, const Storage& map,
                                   detail::ValidationCtx& ctx, char c)
        requires std::is_same_v<Tag, detail::parsing_events_tags::map_key_parsed_some_chars>
    {
        st.searcher.step(c);  // Update search state, don't fail yet
        return true;
    }
    
    // Final validation after key is fully parsed
    template<class Tag, std::size_t Index, class Storage, class KeyType>
    static constexpr bool validate(state<Storage>& st, const Storage& map,
                                   detail::ValidationCtx& ctx, const KeyType& key, std::size_t entry_count) 
        requires std::is_same_v<Tag, detail::parsing_events_tags::map_key_finished>
    {
        auto result = st.searcher.result();
        if (result != sortedKeys.end()) {
            // Found a required key - mark as seen
            std::size_t idx = result->originalIndex;
            st.seen.set(idx);
        }
        // Reset searcher for next key
        st.searcher.reset();
        return true;
    }
    
    
    // Check all required keys were seen
    template<class Tag, std::size_t Index, class Storage>
    static constexpr bool validate(state<Storage>& st, const Storage& map,
                                   detail::ValidationCtx& ctx, std::size_t entry_count) 
            requires std::is_same_v<Tag, detail::parsing_events_tags::map_parsing_finished>                               
    {
        st.searcher.reset();
        if (st.seen.count() != keyCount) {
            ctx.setError(SchemaError::map_missing_required_key, Index);
            return false;
        }
        return true;
    }
    static constexpr std::string_view to_string() {
        return "no required key";
    }
};

template<ConstString... Keys>
struct allowed_keys {
    
    using KeySet = KeySetHelper<Keys...>;
    static constexpr std::size_t keyCount = KeySet::keyCount;
    static constexpr auto& sortedKeys = KeySet::sortedKeys;
    static constexpr std::size_t maxKeyLength = KeySet::maxKeyLength;
    
    template<class Storage>
    struct state {
        AdaptiveNameSearch<keyCount, maxKeyLength> searcher{
            sortedKeys.data(), 
            sortedKeys.data() + sortedKeys.size()
        };
    };
    
    // Incremental validation during key parsing (per-character)
    template<class Tag, std::size_t Index, class Storage>
    static constexpr bool validate(state<Storage>& st, const Storage& map,
                                   detail::ValidationCtx& ctx, char c)
        requires std::is_same_v<Tag, detail::parsing_events_tags::map_key_parsed_some_chars>
    {
        // Feed character to incremental search
        if (!st.searcher.step(c)) {
            // Early rejection: no allowed key matches this prefix
            ctx.setError(SchemaError::map_key_not_allowed, Index);
            st.searcher.reset();
            return false;
        }
        return true;
    }
    
    // Final validation after key is fully parsed
    template<class Tag, std::size_t Index, class Storage, class KeyType>
    static constexpr bool validate(state<Storage>& st, const Storage& map,
                                   detail::ValidationCtx& ctx, const KeyType& key, std::size_t entry_count) 
        requires std::is_same_v<Tag, detail::parsing_events_tags::map_key_finished>
    {
        auto result = st.searcher.result();
        st.searcher.reset();
        if (result == sortedKeys.end()) {
            // Key not in allowed list
            ctx.setError(SchemaError::map_key_not_allowed, Index);
            return false;
        }
        // Reset searcher for next key
        return true;
    }
    static constexpr std::string_view to_string() {
        return "key is not allowed";
    }
};

template<ConstString... Keys>
struct forbidden_keys {
    
    using KeySet = KeySetHelper<Keys...>;
    static constexpr std::size_t keyCount = KeySet::keyCount;
    static constexpr auto& sortedKeys = KeySet::sortedKeys;
    static constexpr std::size_t maxKeyLength = KeySet::maxKeyLength;
    
    template<class Storage>
    struct state {
        AdaptiveNameSearch<keyCount, maxKeyLength> searcher{
            sortedKeys.data(), 
            sortedKeys.data() + sortedKeys.size()
        };
    };
    
    // Incremental validation during key parsing (per-character)
    template<class Tag, std::size_t Index, class Storage>
    static constexpr bool validate(state<Storage>& st, const Storage& map,
                                   detail::ValidationCtx& ctx, char c)
        requires std::is_same_v<Tag, detail::parsing_events_tags::map_key_parsed_some_chars>
    {
        st.searcher.step(c);  // Track matching, but don't fail yet
        return true;
    }
    
    // Final validation after key is fully parsed
    template<class Tag, std::size_t Index, class Storage, class KeyType>
    static constexpr bool validate(state<Storage>& st, const Storage& map,
                                   detail::ValidationCtx& ctx, const KeyType& key, std::size_t entry_count) 
        requires std::is_same_v<Tag, detail::parsing_events_tags::map_key_finished>
    {
        auto result = st.searcher.result();
        st.searcher.reset();
        if (result != sortedKeys.end()) {
            // Key IS in forbidden list - reject!
            ctx.setError(SchemaError::map_key_forbidden, Index);
            return false;
        }
        return true;
    }
    static constexpr std::string_view to_string() {
        return "key is forbidden";
    }

};

} //namespace validators


} // namespace JsonFusion
