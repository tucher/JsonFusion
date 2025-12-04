#pragma once
#include <type_traits>
#include <limits>
#include <utility>
#include <bitset>

#include "options.hpp"
#include "string_search.hpp"
#include "parse_errors.hpp"

namespace JsonFusion {

namespace validators {


namespace validators_detail {


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



template<class... Opts, class Storage>
struct validator_state<options::detail::field_options<Opts...>, Storage> {
    static constexpr std::size_t OptsCount = sizeof...(Opts);
    using OptsTuple   = std::tuple<Opts...>;
    using StatesTuple = std::tuple<option_state_t<Opts, Storage>...>;
    StatesTuple states{};
    template<class Tag, class... Args>
    constexpr bool validate(const Storage& storage, ValidationCtx& ctx, const Args&... args) {
        return validate_impl<Tag>(
            std::make_index_sequence<OptsCount>{},
            storage, ctx, args...
        );
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
        // using State = std::tuple_element_t<I, StatesTuple>;

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
struct string_parsing_finished{};
struct array_item_parsed{};
struct array_parsing_finished{};
struct object_parsing_finished{};
struct descrtuctured_object_parsing_finished{};
struct map_key_finished{};
struct map_value_parsed{};
struct map_entry_parsed{};
struct map_parsing_finished{};

}


/// Helper to compute sorted keys and metadata for key set validators
template<ConstString... Keys>
struct KeySetHelper {
    static constexpr std::size_t keyCount = sizeof...(Keys);

    // Build sorted array of key names at compile time
    static constexpr std::array<string_search::StringDescr, keyCount> sortedKeys = []() consteval {
        std::array<string_search::StringDescr, keyCount> arr;
        std::size_t idx = 0;
        ((arr[idx++] = string_search::StringDescr{Keys.toStringView(), idx}), ...);
        std::ranges::sort(arr, {}, &string_search::StringDescr::name);
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



} //namespace detail


template<auto C>
struct constant {
    static_assert(!is_const_string<decltype(C)>::value, "Use string_constant instead");
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::bool_parsing_finished>
    static constexpr  bool validate(const Storage&  v, validators_detail::ValidationCtx&  ctx) {
        static_assert(std::is_same_v<decltype(C), bool>, "Constant is not bool");
        if(v != C) {
            ctx.setError(SchemaError::wrong_constant_value, Index);
            return false;
        } else {
            return true;
        }
    }
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::number_parsing_finished>
    static constexpr  bool validate(const Storage&  v, validators_detail::ValidationCtx&  ctx) {
        if(v != C) {
            ctx.setError(SchemaError::wrong_constant_value, Index);
            return false;
        } else {
            return true;
        }
    }

    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::string_parsing_finished>
    static constexpr  bool validate(const Storage&  v, validators_detail::ValidationCtx&  ctx, const std::string_view & value) {
        static_assert(is_const_string<decltype(C)>::value, "Constant is not string");
        if(value != C.toStringView()) {
            ctx.setError(SchemaError::wrong_constant_value, Index);
            return false;
        } else {
            return true;
        }
    }
    static constexpr std::string_view to_string() {
        return "constant";
    }
};

template<ConstString C>
struct string_constant {
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::string_parsing_finished>
    static constexpr  bool validate(const Storage&  v, validators_detail::ValidationCtx&  ctx, const std::string_view & value) {
        static_assert(is_const_string<decltype(C)>::value, "Constant is not string");
        if(value != C.toStringView()) {
            ctx.setError(SchemaError::wrong_constant_value, Index);
            return false;
        } else {
            return true;
        }
    }
    static constexpr std::string_view to_string() {
        return "string_constant";
    }
};

// ============================================================================
// String Enum Validation
// ============================================================================

template<ConstString... Values>
struct enum_values {    
    using ValueSet = validators_detail::KeySetHelper<Values...>;
    static constexpr std::size_t valueCount = ValueSet::keyCount;
    static constexpr auto& sortedValues = ValueSet::sortedKeys;
    static constexpr std::size_t maxValueLength = ValueSet::maxKeyLength;
    
    template<class Storage>
    struct state {
        string_search::AdaptiveStringSearch<false, maxValueLength> searcher{
            sortedValues.data(), 
            sortedValues.data() + sortedValues.size()
        };
    };
    

    
    // Final validation after string is fully parsed
    template<class Tag, std::size_t Index, class Storage>
    static constexpr  bool validate(state<Storage>& st, const Storage& str,
                                   validators_detail::ValidationCtx& ctx, const std::string_view & value)
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::string_parsing_finished>
    {
        for(const auto & c: value) {
            if(!st.searcher.step(c))
                break;
        }

        auto result = st.searcher.result();
        st.searcher.reset();
        
        // Check if we have a valid match
        if (result == sortedValues.end()) {
            // String does not match any enum value (or is empty)
            ctx.setError(SchemaError::wrong_constant_value, Index);
            return false;
        } else {
            if (result->name != value) {
                ctx.setError(SchemaError::wrong_constant_value, Index);
                return false;
            } return true;
        }
        
        return true;
    }

    static constexpr std::string_view to_string() {
        return "enum_values";
    }
};

template<auto Min, auto Max>
struct range {
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::number_parsing_finished>
    static constexpr  bool validate(const Storage& val, validators_detail::ValidationCtx&ctx) {
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
        return "range";
    }
};


template<std::size_t N>
struct min_length {
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::string_parsing_finished>
    static constexpr  bool validate(const Storage& val, validators_detail::ValidationCtx&ctx, const std::string_view & value) {
        if(value.size() >= N) {
            return true;
        } else {
            ctx.setError(SchemaError::string_length_out_of_range, Index);
            return false;
        }
    }
    static constexpr std::string_view to_string() {
        return "min_length";
    }
};

template<std::size_t N>
struct max_length {

    // Final validation after string is fully parsed
    template<class Tag, std::size_t Index, class Storage>
    static constexpr  bool validate(const Storage& str,
                                          validators_detail::ValidationCtx& ctx, const std::string_view & value)
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::string_parsing_finished>
    {
        if (value.size() > N) {
            // Early rejection: string is too long
            ctx.setError(SchemaError::string_length_out_of_range, Index);
            return false;
        }
        return true;
    }
    static constexpr std::string_view to_string() {
        return "max_length";
    }
};

template<std::size_t N>
struct min_items {
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::array_parsing_finished>
    static constexpr  bool validate(const Storage& val, validators_detail::ValidationCtx&ctx, std::size_t count) {
        if(count >= N) {
            return true;
        } else {
            ctx.setError(SchemaError::array_items_count_out_of_range, Index);
            return false;
        }
    }
    static constexpr std::string_view to_string() {
        return "min_items";
    }
};

template<std::size_t N>
struct max_items {
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::array_item_parsed>
    static constexpr  bool validate(const Storage& val, validators_detail::ValidationCtx&ctx, std::size_t count) {
        if(count <= N) {
            return true;
        } else {
            ctx.setError(SchemaError::array_items_count_out_of_range, Index);
            return false;
        }
    }
    static constexpr std::string_view to_string() {
        return "max_items";
    }
};

template <ConstString ... NotRequiredNames>
struct not_required {
    template<class Tag, std::size_t Index, class Storage, class FH>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::object_parsing_finished>
    static constexpr  bool validate(const Storage& val, validators_detail::ValidationCtx&ctx, const std::bitset<FH::fieldsCount> & seen, const FH&) {
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
        return "not_required";
    }
};

// ============================================================================
// Map/Object Property Count Validators
// ============================================================================

template<std::size_t N>
struct min_properties {
    static constexpr std::size_t value = N;
    
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::map_parsing_finished>
    static constexpr  bool validate(const Storage& val, validators_detail::ValidationCtx& ctx, std::size_t count) {
        if (count >= N) {
            return true;
        } else {
            ctx.setError(SchemaError::map_properties_count_out_of_range, Index);
            return false;
        }
    }
    static constexpr std::string_view to_string() {
        return "min_properties";
    }
};

template<std::size_t N>
struct max_properties {
    static constexpr std::size_t value = N;
    
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::map_entry_parsed>
    static constexpr  bool validate(const Storage& val, validators_detail::ValidationCtx& ctx, std::size_t count) {
        if (count <= N) {
            return true;
        } else {
            ctx.setError(SchemaError::map_properties_count_out_of_range, Index);
            return false;
        }
    }
    static constexpr std::string_view to_string() {
        return "max_properties";
    }
};

// ============================================================================
// Map Key Validation
// ============================================================================

template<std::size_t N>
struct min_key_length {
    static constexpr std::size_t value = N;
    
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::map_key_finished>
    static constexpr  bool validate(const Storage& val, validators_detail::ValidationCtx& ctx, const std::string_view & key) {
        
        if (key.size() >= N) {
            return true;
        } else {
            ctx.setError(SchemaError::map_key_length_out_of_range, Index);
            return false;
        }
    }
    static constexpr std::string_view to_string() {
        return "min_key_length";
    }
};

template<std::size_t N>
struct max_key_length {
    static constexpr std::size_t value = N;
    
    template<class Tag, std::size_t Index, class Storage>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::map_key_finished>
    static constexpr  bool validate(const Storage& val, validators_detail::ValidationCtx& ctx, const std::string_view & key) {
        
        if (key.size() <= N) {
            return true;
        } else {
            ctx.setError(SchemaError::map_key_length_out_of_range, Index);
            return false;
        }
    }
    static constexpr std::string_view to_string() {
        return "max_key_length";
    }
};

template<ConstString... Keys>
struct required_keys {
    
    using KeySet = validators_detail::KeySetHelper<Keys...>;
    static constexpr std::size_t keyCount = KeySet::keyCount;
    static constexpr auto& sortedKeys = KeySet::sortedKeys;
    static constexpr std::size_t maxKeyLength = KeySet::maxKeyLength;
    
    template<class Storage>
    struct state {
        std::bitset<keyCount> seen{};  // Track which required keys were parsed
        string_search::AdaptiveStringSearch<false, maxKeyLength> searcher{
            sortedKeys.data(), 
            sortedKeys.data() + sortedKeys.size()
        };
    };
    

    
    // Final validation after key is fully parsed
    template<class Tag, std::size_t Index, class Storage>
    static constexpr  bool validate(state<Storage>& st, const Storage& map,
                                   validators_detail::ValidationCtx& ctx, const std::string_view & key)
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::map_key_finished>
    {
        for(const auto & c: key) {
            if(!st.searcher.step(c))
                break;
        }
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
    static constexpr  bool validate(state<Storage>& st, const Storage& map,
                                   validators_detail::ValidationCtx& ctx, std::size_t entry_count)
            requires std::is_same_v<Tag, validators_detail::parsing_events_tags::map_parsing_finished>
    {
        st.searcher.reset();
        if (st.seen.count() != keyCount) {
            ctx.setError(SchemaError::map_missing_required_key, Index);
            return false;
        }
        return true;
    }
    static constexpr std::string_view to_string() {
        return "required_keys";
    }
};

template<ConstString... Keys>
struct allowed_keys {
    
    using KeySet = validators_detail::KeySetHelper<Keys...>;
    static constexpr std::size_t keyCount = KeySet::keyCount;
    static constexpr auto& sortedKeys = KeySet::sortedKeys;
    static constexpr std::size_t maxKeyLength = KeySet::maxKeyLength;
    
    template<class Storage>
    struct state {
        string_search::AdaptiveStringSearch<false, maxKeyLength> searcher{
            sortedKeys.data(), 
            sortedKeys.data() + sortedKeys.size()
        };
    };

    
    // Final validation after key is fully parsed
    template<class Tag, std::size_t Index, class Storage>
    static constexpr  bool validate(state<Storage>& st, const Storage& map,
                                   validators_detail::ValidationCtx& ctx, const std::string_view & key)
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::map_key_finished>
    {
        for(const auto & c: key) {
            if(!st.searcher.step(c))
                break;
        }

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
        return "allowed_keys";
    }
};

template<ConstString... Keys>
struct forbidden_keys {
    
    using KeySet = validators_detail::KeySetHelper<Keys...>;
    static constexpr std::size_t keyCount = KeySet::keyCount;
    static constexpr auto& sortedKeys = KeySet::sortedKeys;
    static constexpr std::size_t maxKeyLength = KeySet::maxKeyLength;
    
    template<class Storage>
    struct state {
        string_search::AdaptiveStringSearch<false, maxKeyLength> searcher{
            sortedKeys.data(), 
            sortedKeys.data() + sortedKeys.size()
        };
    };

    
    // Final validation after key is fully parsed
    template<class Tag, std::size_t Index, class Storage>
    static constexpr  bool validate(state<Storage>& st, const Storage& map,
                                   validators_detail::ValidationCtx& ctx, const std::string_view & key)
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::map_key_finished>
    {
        for(const auto & c: key) {
            if(!st.searcher.step(c))
                break;
        }

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
        return "forbidden_keys";
    }

};

} //namespace validators


} // namespace JsonFusion
