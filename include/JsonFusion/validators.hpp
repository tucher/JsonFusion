#pragma once
#include <cstdint>
#include <type_traits>
#include <limits>
#include <utility>
#include <bitset>

#include "options.hpp"
#include "struct_introspection.hpp"

namespace JsonFusion {

namespace validators {


enum class SchemaError : std::uint64_t {
    none                            = 0,
    number_out_of_range             = 1ull << 0,
    string_length_out_of_range      = 1ull << 1,
    array_items_count_out_of_range  = 1ull << 2,
    missing_required_fields         = 1ull << 3,
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


template <class Opts>
struct validator {

};

template<>
struct validator<options::detail::no_options> {
    template<class ValidatorsTag, class ObjT, class ... Args>
    static constexpr bool validate(const ObjT & storage, ValidationCtx & ctx, const Args & ... args) {
        return true;
    }
};

template<class T, class... Opts>
struct validator<options::detail::field_options<T, Opts...>> {
    using underlying_type = T;
    using GlobalOpts = options::detail::field_options<T, Opts...>;

    template<class ValidatorsTag, class ObjT, class ... Args>
    static constexpr bool validate(const ObjT & storage, ValidationCtx & ctx, const Args & ... args) {
        auto callOne = [&]<class Opt>() {
            if constexpr(std::same_as<typename Opt::tag, ValidatorsTag>) {
                return Opt::validate(storage, ctx, args...);
            } else {
                return true;
            }
        };
        bool ok = true;
        (void)std::initializer_list<int>{
            (ok = ok && callOne.template operator()<Opts>(), 0)...
        };
        return ok;
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

}


template<auto Min, auto Max>
struct range {
    using tag = parsing_events_tags::number_parsing_finished;

    template<class Storage>
    static constexpr bool validate(const Storage& val, ValidationCtx&ctx) {
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
    using tag = parsing_events_tags::string_parsing_finished;
    static constexpr std::size_t value = N;
    template<class Storage>
    static constexpr bool validate(const Storage& val, ValidationCtx&ctx, std::size_t size) {
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
    using tag = parsing_events_tags::string_parsed_some_chars;
    static constexpr std::size_t value = N;
    template<class Storage>
    static constexpr bool validate(const Storage& val, ValidationCtx&ctx, std::size_t size) {
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
    using tag = parsing_events_tags::array_parsing_finished;
    template<class Storage>
    static constexpr bool validate(const Storage& val, ValidationCtx&ctx, std::size_t count) {
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
    using tag = parsing_events_tags::array_item_parsed;
    template<class Storage>
    static constexpr bool validate(const Storage& val, ValidationCtx&ctx, std::size_t count) {
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
    using tag = parsing_events_tags::object_parsing_finished;

    template<class Storage, class FH>
    static constexpr bool validate(const Storage& val, ValidationCtx&ctx, const std::bitset<FH::fieldsCount> & seen, const FH&) {
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



} //namespace validators


} // namespace JsonFusion
