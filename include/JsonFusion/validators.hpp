#pragma once
#include <cstdint>
#include <type_traits>
#include <limits>
#include <utility>
#include <bitset>

#include "options.hpp"
#include "struct_introspection.hpp"
#include <iostream>
#include <map>
namespace JsonFusion {

namespace validators {

enum class SchemaError : std::uint64_t {
    none                            = 0,
    number_out_of_range             = 1ull << 0,
    string_length_out_of_range      = 1ull << 1,
    array_items_count_out_of_range  = 1ull << 2,
    missing_required_fields         = 1ull << 3,
    map_properties_count_out_of_range = 1ull << 4,
    map_key_length_out_of_range     = 1ull << 5,
    wrong_constant_value            = 1ull << 6,
    map_key_not_allowed             = 1ull << 7,
    map_key_forbidden               = 1ull << 9,
    map_missing_required_key        = 1ull << 10,
    // … more, all 1 << N
};

using SchemaErrorMask = std::uint64_t;


struct ValidationResult {
    SchemaErrorMask  schema_mask_  = 0;
    constexpr operator bool() const {
        return schema_mask_ == 0;
    }

    constexpr SchemaErrorMask schema_errors() const {
        return schema_mask_;
    }

    constexpr bool hasSchemaError(SchemaError e) const {
        return schema_mask_ &
               static_cast<SchemaErrorMask>(e);
    }

};


namespace detail {



struct ValidationCtx {
    SchemaErrorMask  schema_mask_  = 0;
    constexpr void addSchemaError(SchemaError e) {
        schema_mask_ |= static_cast<SchemaErrorMask>(e);
        // optionally: if there was no "hard" parse error yet,
        // you might set error_ = ParseError::SCHEMA_ERROR;
    }
    constexpr ValidationResult result() {
        return ValidationResult{schema_mask_};
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
    using OptsTuple = std::tuple<detail::option_state_t<Opts, Storage>...>;
    OptsTuple states{};

    template<class Tag, class... Args>
    constexpr bool validate(const Storage& storage, ValidationCtx& ctx, const Args&... args) {
        bool ok = true;
        std::apply(
            [&](auto&... st) {
                // iterate over options, in lockstep with states
                ((ok = ok && call_one<Tag, Opts>(st, storage, ctx, args...)), ...);
            },
            states
            );
        return ok;
    }

private:
    template<class Tag, class Opt, class State, class... Args>
    static constexpr bool call_one(
        State&        st,
        const Storage& storage,
        ValidationCtx&          ctx,
        const Args&... args
        )
    {
        if constexpr (!std::is_same_v<State, empty_state>) {
            if constexpr (requires { Opt::template validate<Tag>(Tag{}, st, storage, ctx, args...); }) {
                return Opt::template validate<Tag>(Tag{}, st, storage, ctx, args...);
            } else {
                return true;
            }

        } else {

            // Stateless option
            if constexpr (requires { Opt::template validate<Tag>(Tag{}, storage, ctx, args...); }) {
                return Opt::template validate<Tag>(Tag{}, storage, ctx, args...);
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
struct map_key_parsed_some_chars{};
struct map_key_finished{};
struct map_value_parsed{};
struct map_entry_parsed{};
struct map_parsing_finished{};

}
} //namespace detail

template<auto C>
struct constant {
    struct notag{}; // TODO remove!!!
    using tag = notag; // or multiple tags


    template<class Tag, class Storage>
    static constexpr bool validate(const Tag &,  const Storage&  v, detail::ValidationCtx&  ctx) {
        if constexpr(std::is_same_v<Tag, detail::parsing_events_tags::bool_parsing_finished>) {
            if(v != C) {
                ctx.addSchemaError(SchemaError::wrong_constant_value);
                return false;
            } else {
                return true;
            }
        } else if constexpr(false) { //TODO add others
        }else {
            return true;
        }
    }
};

template<auto Min, auto Max>
struct range {
    using tag = detail::parsing_events_tags::number_parsing_finished;

    template<class Storage>
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
            ctx.addSchemaError(SchemaError::number_out_of_range);
            return false;
        } else {
            return true;
        }
    }
};


template<std::size_t N>
struct min_length {
    using tag = detail::parsing_events_tags::string_parsing_finished;

    template<class Storage>
    static constexpr bool validate(const Storage& val, detail::ValidationCtx&ctx, std::size_t size) {
        if(size >= N) {
            return true;
        } else {
            ctx.addSchemaError(SchemaError::string_length_out_of_range);
            return false;
        }
    }
};

template<std::size_t N>
struct max_length {
    using tag = detail::parsing_events_tags::string_parsed_some_chars;

    template<class Storage>
    static constexpr bool validate(const Storage& val, detail::ValidationCtx&ctx, std::size_t size) {
        if(size <= N) {
            return true;
        } else {
            ctx.addSchemaError(SchemaError::string_length_out_of_range);
            return false;
        }
    }
};

template<std::size_t N>
struct min_items {
    using tag = detail::parsing_events_tags::array_parsing_finished;
    template<class Storage>
    static constexpr bool validate(const Storage& val, detail::ValidationCtx&ctx, std::size_t count) {
        if(count >= N) {
            return true;
        } else {
            ctx.addSchemaError(SchemaError::array_items_count_out_of_range);
            return false;
        }
    }
};

template<std::size_t N>
struct max_items {
    using tag = detail::parsing_events_tags::array_item_parsed;
    template<class Storage>
    static constexpr bool validate(const Storage& val, detail::ValidationCtx&ctx, std::size_t count) {
        if(count <= N) {
            return true;
        } else {
            ctx.addSchemaError(SchemaError::array_items_count_out_of_range);
            return false;
        }
    }
};

template <ConstString ... NotRequiredNames>
struct not_required {
    using tag = detail::parsing_events_tags::object_parsing_finished;

    template<class Storage, class FH>
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
            ctx.addSchemaError(SchemaError::missing_required_fields);
            return false;
        }
        return true;
    }
};

// ============================================================================
// Map/Object Property Count Validators
// ============================================================================

template<std::size_t N>
struct min_properties {
    using tag = detail::parsing_events_tags::map_parsing_finished;
    static constexpr std::size_t value = N;
    
    template<class Tag, class Storage>
    static constexpr bool validate(const Tag&, const Storage& val, detail::ValidationCtx& ctx, std::size_t count) {
        if constexpr (std::is_same_v<Tag, detail::parsing_events_tags::map_parsing_finished>) {
            if (count >= N) {
                return true;
            } else {
                ctx.addSchemaError(SchemaError::map_properties_count_out_of_range);
                return false;
            }
        } else {
            return true;
        }
    }
};

template<std::size_t N>
struct max_properties {
    using tag = detail::parsing_events_tags::map_entry_parsed;
    static constexpr std::size_t value = N;
    
    template<class Storage>
    static constexpr bool validate(const Storage& val, detail::ValidationCtx& ctx, std::size_t count) {
        if (count <= N) {
            return true;
        } else {
            ctx.addSchemaError(SchemaError::map_properties_count_out_of_range);
            return false;
        }
    }
};

// ============================================================================
// Map Key Validation
// ============================================================================

template<std::size_t N>
struct min_key_length {
    using tag = detail::parsing_events_tags::map_key_finished;
    static constexpr std::size_t value = N;
    
    template<class Storage, class KeyType>
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
            ctx.addSchemaError(SchemaError::map_key_length_out_of_range);
            return false;
        }
    }
};

template<std::size_t N>
struct max_key_length {
    using tag = detail::parsing_events_tags::map_key_finished;
    static constexpr std::size_t value = N;
    
    template<class Storage, class KeyType>
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
            ctx.addSchemaError(SchemaError::map_key_length_out_of_range);
            return false;
        }
    }
};

template<ConstString ... Keys>
struct required_keys {
    struct notag{}; // TODO remove!!!
    using tag = notag; // or multiple tags

    template<class Storage>
    struct state {
        // anything you need for this option *for this Storage*
        // std::bitset<sizeof...(Keys)> seen{};
        // maybe also a fast mapping from key->index, precomputed in a consteval helper
        std::map<std::string, bool> seen = [](){
            std::map<std::string, bool>  ret;
            ((ret.emplace(Keys.toStringView(), false)), ...);
            return ret;
        }();
    };

    template<class Tag, class Storage>
    static constexpr bool validate(const Tag &, state<Storage>& st, const Storage&  map, detail::ValidationCtx&  ctx, std::size_t entry_count) {
        if constexpr(std::is_same_v<Tag, detail::parsing_events_tags::map_parsing_finished>) {
            for(auto[_,v]: st.seen) {
                if(!v) {
                    ctx.addSchemaError(SchemaError::map_missing_required_key);
                    return false;
                }
            }
            return true;
        } else {
            return true;
        }
    }
    template<class Tag, class Storage>
    static constexpr bool validate(const Tag &, state<Storage>& st, const Storage&  map, detail::ValidationCtx&  ctx, const std::string & key, std::size_t entry_count) {
        if constexpr (std::is_same_v<Tag, detail::parsing_events_tags::map_key_finished>) {
            if(st.seen.contains(key)) {
                st.seen[key] = true;
            }
            return true;
        } else {
            return true;
        }
    }
};

template<ConstString ... Keys>
struct allowed_keys {
    struct notag{}; // TODO remove!!!
    using tag = notag; // or multiple tags

    template<class Storage>
    struct state {
        std::map<std::string, bool> keys_set = [](){
            std::map<std::string, bool>  ret;
            ((ret.emplace(Keys.toStringView(), false)), ...);
            return ret;
        }();
    };


    template<class Tag, class Storage>
    static constexpr bool validate(const Tag &, state<Storage>& st, const Storage&  map, detail::ValidationCtx&  ctx, const std::string & key, std::size_t entry_count) {
        if constexpr (std::is_same_v<Tag, detail::parsing_events_tags::map_key_finished>) {
            if(!st.keys_set.contains(key)) {
                ctx.addSchemaError(SchemaError::map_key_not_allowed);
                return false;
            }
            return true;
        } else {
            return true;
        }
    }
};

template<ConstString ... Keys>
struct forbidden_keys {
    struct notag{}; // TODO remove!!!
    using tag = notag; // or multiple tags

    template<class Storage>
    struct state {
        std::map<std::string, bool> keys_set = [](){
            std::map<std::string, bool>  ret;
            ((ret.emplace(Keys.toStringView(), false)), ...);
            return ret;
        }();
    };


    template<class Tag, class Storage>
    static constexpr bool validate(const Tag &, state<Storage>& st, const Storage&  map, detail::ValidationCtx&  ctx, const std::string & key, std::size_t entry_count) {
        if constexpr (std::is_same_v<Tag, detail::parsing_events_tags::map_key_finished>) {
            if(st.keys_set.contains(key)) {
                ctx.addSchemaError(SchemaError::map_key_forbidden);
                return false;
            }
            return true;
        } else {
            return true;
        }
    }
};

} //namespace validators


} // namespace JsonFusion
