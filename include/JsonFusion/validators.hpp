#pragma once
#include <type_traits>
#include <limits>
#include <utility>
#include <bitset>
#include <functional>

#include "options.hpp"

namespace JsonFusion {
namespace validators {
namespace validators_detail {

// Constexpr-compatible absolute value for floating-point
template<typename T>
constexpr T constexpr_abs(T value) {
    return value < T(0) ? -value : value;
}

} // namespace validators_detail
} // namespace validators
} // namespace JsonFusion
#include "string_search.hpp"
#include "errors.hpp"

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
struct validator_state<options::detail::field_options<OptionsPack<Opts...>>, Storage> {
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
    
    // Generic max property aggregation across all validators
    template<class PropertyTag>
    static constexpr std::size_t max_property() {
        return max_property_impl<PropertyTag>(std::make_index_sequence<OptsCount>{});
    }
    
    // Generic min property aggregation across all validators
    template<class PropertyTag>
    static constexpr std::size_t min_property() {
        return min_property_impl<PropertyTag>(std::make_index_sequence<OptsCount>{});
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
    
    // Helper to query a single validator's property
    template<class PropertyTag, std::size_t Index, class Opt>
    static constexpr std::size_t get_validator_property() {
        if constexpr (requires { Opt::template get_property<PropertyTag, Index>(); }) {
            return Opt::template get_property<PropertyTag, Index>();
        } else {
            return 0; // Validator doesn't provide this property
        }
    }
    
    // Implementation of max_property
    template<class PropertyTag, std::size_t... I>
    static constexpr std::size_t max_property_impl(std::index_sequence<I...>) {
        std::size_t result = 0;
        ((result = std::max(result, get_validator_property<PropertyTag, I, std::tuple_element_t<I, OptsTuple>>())), ...);
        return result;
    }
    
    // Implementation of min_property
    template<class PropertyTag, std::size_t... I>
    static constexpr std::size_t min_property_impl(std::index_sequence<I...>) {
        std::size_t result = std::numeric_limits<std::size_t>::max();
        ((result = std::min(result, get_validator_property<PropertyTag, I, std::tuple_element_t<I, OptsTuple>>())), ...);
        return (result == std::numeric_limits<std::size_t>::max()) ? 0 : result;
    }
};

template<class Storage>
struct validator_state<options::detail::no_options, Storage> {
    template<class Tag, class Ctx, class... Args>
    constexpr bool validate(const Storage&, Ctx&, const Args&...) {
        return true;
    }
    
    // No validators, so all properties return 0
    template<class PropertyTag>
    static constexpr std::size_t max_property() {
        return 0;
    }
    
    template<class PropertyTag>
    static constexpr std::size_t min_property() {
        return 0;
    }
};





namespace parsing_events_tags {

struct bool_parsing_finished{};
struct number_parsing_finished{};
struct string_parsing_finished{};
struct array_item_parsed{};
struct array_parsing_finished{};
struct object_field_parsed{};
struct object_parsing_finished{};
struct excess_field_occured{};
struct descrtuctured_object_parsing_finished{};
struct map_key_finished{};
struct map_value_parsed{};
struct map_entry_parsed{};
struct map_parsing_finished{};

}

// Tags for querying parsing constraint properties from validators
namespace parsing_constraint_properties_tags {
    struct max_excess_field_name_length {};
    // Early rejection properties for upper-bound validators
    struct max_string_length {};
    struct max_array_items {};
    struct max_map_properties {};
    struct max_map_key_length {};
}

namespace validators_options_tags {
    struct range_tag{};
    struct min_length_tag{};
    struct max_length_tag{};
    struct enum_values_tag{};
    struct min_items_tag{};
    struct max_items_tag{};
    struct constant_tag{};
    struct string_constant_tag{};
    struct not_required_tag{};
    struct required_tag{};
    struct forbidden_tag{};
    struct min_properties_tag{};
    struct max_properties_tag{};
    struct min_key_length_tag{};
    struct max_key_length_tag{};
    struct required_keys_tag{};
    struct allowed_keys_tag{};
    struct forbidden_keys_tag{};
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
    constexpr static auto value = C;
    using tag = validators_detail::validators_options_tags::constant_tag;
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
        if constexpr (std::is_floating_point_v<Storage>) {
            static_assert(std::is_floating_point_v<decltype(C)>, "Constant is not floating point");
            
            // Reject NaN and Infinity - not valid JSON numbers per RFC 8259
            static_assert(C == C, "NaN is not a valid JSON number"); // NaN != NaN
            static_assert(C != std::numeric_limits<decltype(C)>::infinity() && 
                          C != -std::numeric_limits<decltype(C)>::infinity(), 
                          "Infinity is not a valid JSON number");
            
            // Require exact type match - no narrowing or widening
            // (Widening float→double creates precision mismatches: 3.14f != 3.14 as double)
            static_assert(std::is_same_v<decltype(C), Storage>, 
                          "Constant type must exactly match field type (use 3.14f for float, 3.14 for double)");
            
            // Cast to Storage type for comparison to handle widening conversions
            constexpr Storage ConstValue = static_cast<Storage>(C);
            Storage diff = validators_detail::constexpr_abs(v - ConstValue);
            Storage absV = validators_detail::constexpr_abs(v);
            Storage absConst = validators_detail::constexpr_abs(ConstValue);
            Storage maxVal = (absV > absConst) ? absV : absConst;
            
            if (diff <= std::numeric_limits<Storage>::epsilon() * maxVal * Storage(2.0)
            || diff < std::numeric_limits<Storage>::min()) {
                return true;
            } else {
                ctx.setError(SchemaError::wrong_constant_value, Index);
                return false;
            }
        } else {
            static_assert(std::is_integral_v<decltype(C)>, "Constant is not integral");
            if constexpr (std::is_unsigned_v<Storage>) {
                static_assert(static_cast<std::make_unsigned_t<decltype(C)>>(C) <= std::numeric_limits<Storage>::max(), "Constant is out of range");
            } else {
                static_assert(C >= std::numeric_limits<Storage>::min() && C <= std::numeric_limits<Storage>::max(), "Constant is out of range");
            }

            if(v != static_cast<Storage>(C)) {
                ctx.setError(SchemaError::wrong_constant_value, Index);
                return false;
            } else {    
                return true;
            }
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
    constexpr static auto value = C;
    using tag = validators_detail::validators_options_tags::string_constant_tag;
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
    using tag = validators_detail::validators_options_tags::enum_values_tag;
    using ValueSet = validators_detail::KeySetHelper<Values...>;
    static constexpr std::size_t valueCount = ValueSet::keyCount;
    static constexpr auto& sortedValues = ValueSet::sortedKeys;
    static constexpr std::size_t maxValueLength = ValueSet::maxKeyLength;
    
    template<class Storage>
    struct state {
        string_search::AdaptiveStringSearch<maxValueLength> searcher{
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
    constexpr static auto min = Min;
    constexpr static auto max = Max;
    using tag = validators_detail::validators_options_tags::range_tag;

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
    constexpr static std::size_t value = N;
    using tag = validators_detail::validators_options_tags::min_length_tag;
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
    constexpr static std::size_t value = N;
    using tag = validators_detail::validators_options_tags::max_length_tag;
    
    // Property query for early rejection
    template<class PropertyTag, std::size_t Index>
    static constexpr std::size_t get_property() {
        if constexpr (std::is_same_v<PropertyTag, 
            validators_detail::parsing_constraint_properties_tags::max_string_length>) {
            return N;
        } else {
            return 0;
        }
    }
    
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
    constexpr static std::size_t value = N;
    using tag = validators_detail::validators_options_tags::min_items_tag;
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
    constexpr static std::size_t value = N;
    using tag = validators_detail::validators_options_tags::max_items_tag;
    
    // Property query for early rejection
    template<class PropertyTag, std::size_t Index>
    static constexpr std::size_t get_property() {
        if constexpr (std::is_same_v<PropertyTag, 
            validators_detail::parsing_constraint_properties_tags::max_array_items>) {
            return N;
        } else {
            return 0;
        }
    }
    
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
    using tag = validators_detail::validators_options_tags::not_required_tag;
    static constexpr std::array<std::string_view, sizeof...(NotRequiredNames)> values = []() consteval {
        std::array<std::string_view, sizeof...(NotRequiredNames)> arr;
        std::size_t idx = 0;
        ((arr[idx++] = NotRequiredNames.toStringView()), ...);
        return arr;
    }();

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

template <ConstString ... RequiredNames>
struct required {
    using tag = validators_detail::validators_options_tags::required_tag;

    static constexpr std::array<std::string_view, sizeof...(RequiredNames)> values = []() consteval {
        std::array<std::string_view, sizeof...(RequiredNames)> arr;
        std::size_t idx = 0;
        ((arr[idx++] = RequiredNames.toStringView()), ...);
        return arr;
    }();

    template<class Tag, std::size_t Index, class Storage, class FH>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::object_parsing_finished>
    static constexpr  bool validate(const Storage& val, validators_detail::ValidationCtx&ctx, const std::bitset<FH::fieldsCount> & seen, const FH&) {
        static_assert(
            ((FH::indexInSortedByName(RequiredNames.toStringView()) != -1) &&...),
            "Fields in 'required' are not presented in json model of object, check c++ fields names or 'key' annotations");

        // Builds the required fields mask at compile time: requiredMask[i] = true if field i  in required list
        // This is computed  per template instantiation, not at runtime
        constexpr auto requiredMask = []() consteval {
            std::bitset<FH::fieldsCount> mask{};
            mask.reset(); // all not required by default
            // Mark these as required
            ((mask.set(FH::indexInSortedByName(RequiredNames.toStringView()))), ...);
            return mask;
        }();

        if ((seen & requiredMask) != requiredMask) {
            ctx.setError(SchemaError::missing_required_fields, Index);
            return false;
        }
        return true;
    }
    static constexpr std::string_view to_string() {
        return "required";
    }
};

template <ConstString ... ForbiddenNames>
struct forbidden {
    using tag = validators_detail::validators_options_tags::forbidden_tag;
    static constexpr std::array<std::string_view, sizeof...(ForbiddenNames)> values = []() consteval {
        std::array<std::string_view, sizeof...(ForbiddenNames)> arr;
        std::size_t idx = 0;
        ((arr[idx++] = ForbiddenNames.toStringView()), ...);
        return arr;
    }();
    
    // Property queries - return max forbidden field name length
    template<class PropertyTag, std::size_t Index>
    static constexpr std::size_t get_property() {
        if constexpr (std::is_same_v<PropertyTag, 
            validators_detail::parsing_constraint_properties_tags::max_excess_field_name_length>) {
            // Calculate max forbidden field name length at compile time
            std::size_t max_len = 0;
            ((max_len = std::max(max_len, ForbiddenNames.toStringView().size())), ...);
            return max_len;
        } else {
            return 0;
        }
    }

    template<class Tag, std::size_t Index, class Storage, class FH>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::object_field_parsed>
    static constexpr  bool validate(const Storage& val, validators_detail::ValidationCtx&ctx, std::size_t arrayIndex, const FH&) {
        // Note: Unlike 'required', we don't assert that forbidden fields exist in the C++ struct.
        // The forbidden validator can forbid fields that only appear in JSON (when allow_excess_fields is used).
        
        // Builds the forbidden fields mask at compile time: forbiddenMask[i] = true if field i is in forbidden list
        // This is computed per template instantiation, not at runtime
        constexpr auto forbiddenMask = []() consteval {
            std::bitset<FH::fieldsCount> mask{};
            mask.reset(); // all not forbidden by default
            // Mark these as forbidden (only if they exist in the struct)
            ((FH::indexInSortedByName(ForbiddenNames.toStringView()) != -1 
                ? mask.set(FH::indexInSortedByName(ForbiddenNames.toStringView())) 
                : mask), ...);
            return mask;
        }();

        if (forbiddenMask[arrayIndex]) {
            ctx.setError(SchemaError::forbidden_fields, Index);
            return false;
        }
        return true;
    }
    
    // Validate excess fields (fields not in C++ struct) - check if they are forbidden
    template<class Tag, std::size_t Index, class Storage, class FH>
        requires std::is_same_v<Tag, validators_detail::parsing_events_tags::excess_field_occured>
    static constexpr  bool validate(const Storage& val, validators_detail::ValidationCtx&ctx, std::string_view fieldName, const FH&) {
        // Check if this excess field name matches any forbidden name
        for (const auto& forbiddenName : values) {
            if (fieldName == forbiddenName) {
                ctx.setError(SchemaError::forbidden_fields, Index);
                return false;
            }
        }
        return true;
    }
    
    static constexpr std::string_view to_string() {
        return "forbidden";
    }
};

// ============================================================================
// Map/Object Property Count Validators
// ============================================================================

template<std::size_t N>
struct min_properties {
    static constexpr std::size_t value = N;
    using tag = validators_detail::validators_options_tags::min_properties_tag;
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
    using tag = validators_detail::validators_options_tags::max_properties_tag;
    
    // Property query for early rejection
    template<class PropertyTag, std::size_t Index>
    static constexpr std::size_t get_property() {
        if constexpr (std::is_same_v<PropertyTag, 
            validators_detail::parsing_constraint_properties_tags::max_map_properties>) {
            return N;
        } else {
            return 0;
        }
    }
    
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
    using tag = validators_detail::validators_options_tags::min_key_length_tag;
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
    using tag = validators_detail::validators_options_tags::max_key_length_tag;
    
    // Property query for early rejection
    template<class PropertyTag, std::size_t Index>
    static constexpr std::size_t get_property() {
        if constexpr (std::is_same_v<PropertyTag, 
            validators_detail::parsing_constraint_properties_tags::max_map_key_length>) {
            return N;
        } else {
            return 0;
        }
    }
    
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
    using tag = validators_detail::validators_options_tags::required_keys_tag;
    using KeySet = validators_detail::KeySetHelper<Keys...>;
    static constexpr std::size_t keyCount = KeySet::keyCount;
    static constexpr auto& sortedKeys = KeySet::sortedKeys;
    static constexpr std::size_t maxKeyLength = KeySet::maxKeyLength;
    
    template<class Storage>
    struct state {
        std::bitset<keyCount> seen{};  // Track which required keys were parsed
        string_search::AdaptiveStringSearch<maxKeyLength> searcher{
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
    using tag = validators_detail::validators_options_tags::allowed_keys_tag;
    using KeySet = validators_detail::KeySetHelper<Keys...>;
    static constexpr std::size_t keyCount = KeySet::keyCount;
    static constexpr auto& sortedKeys = KeySet::sortedKeys;
    static constexpr std::size_t maxKeyLength = KeySet::maxKeyLength;
    
    template<class Storage>
    struct state {
        string_search::AdaptiveStringSearch<maxKeyLength> searcher{
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
    using tag = validators_detail::validators_options_tags::forbidden_keys_tag;
    using KeySet = validators_detail::KeySetHelper<Keys...>;
    static constexpr std::size_t keyCount = KeySet::keyCount;
    static constexpr auto& sortedKeys = KeySet::sortedKeys;
    static constexpr std::size_t maxKeyLength = KeySet::maxKeyLength;
    
    template<class Storage>
    struct state {
        string_search::AdaptiveStringSearch<maxKeyLength> searcher{
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

namespace detail {
template<class T>
struct always_false : std::false_type {};
}

// Warning: be really careful with inlining: mark the function as noinline if it is not a small function

template<class PhaseTag, auto Fn>
struct fn_validator {
    template<class Tag, std::size_t Index, class Storage, class... PhaseArgs>
    static constexpr bool validate(
        const Storage& val,
        validators_detail::ValidationCtx& ctx,
        const PhaseArgs&... args
        )
    {
        // Only run on the phase we care about.
        if constexpr (!std::is_same_v<Tag, PhaseTag>) {
            (void)val; (void)ctx; (void)sizeof...(PhaseArgs);
            return true;
        } else {
            // 1) Full signature: (val, ctx, phase args...)
            if constexpr (std::is_invocable_r_v<bool,
                                                decltype(Fn),
                                                const Storage&,
                                                validators_detail::ValidationCtx&,
                                                const PhaseArgs&...>) {
                return std::invoke(Fn, val, ctx, args...);

                // 2) Without ValidationCtx (user doesn’t care about it)
            } else if constexpr (std::is_invocable_r_v<bool,
                                                       decltype(Fn),
                                                       const Storage&,
                                                       const PhaseArgs&...>) {
                const bool ok = std::invoke(Fn, val, args...);
                // If they didn’t set an error but returned false,
                // you can optionally set a generic error here:
                if (!ok && ctx.m_error == SchemaError::none) {
                    ctx.setError(SchemaError::user_defined_fn_validator_error, Index);
                }
                return ok;

                // 3) With Index as a constexpr value wrapper if needed:
            } else if constexpr (std::is_invocable_r_v<bool,
                                                       decltype(Fn),
                                                       std::integral_constant<std::size_t, Index>,
                                                       const Storage&,
                                                       validators_detail::ValidationCtx&,
                                                       const PhaseArgs&...>) {
                return std::invoke(Fn,
                                   std::integral_constant<std::size_t, Index>{},
                                   val,
                                   ctx,
                                   args...);
            } else {
                static_assert(
                    detail::always_false<decltype(Fn)>::value,
                    "[[ JsonFusion ]] fn_validator: callable has unsupported signature"
                    );
            }
        }
    }

    static constexpr std::string_view to_string() {
        return "fn_validator";
    }
};

} //namespace validators


} // namespace JsonFusion
