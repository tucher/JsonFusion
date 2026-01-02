#pragma once
#include <variant>

#include <JsonFusion/generic_transformers.hpp>
#include <JsonFusion/options.hpp>
#include <JsonFusion/static_schema.hpp>
#include <JsonFusion/wire_sink.hpp>

namespace JsonFusion {
namespace transformers {

// Working proof-of-concept for std::variant support via transformers.
// 
// Note: Without a discriminator field or structural hints, the generic case
// requires attempting to parse as each alternative type (N attempts). This
// implementation uses "exactly one match" semantics (oneOf), matching std::variant's
// "holds exactly one value" semantics.
//
// For production use with discriminators, custom resolution strategies, or better
// error diagnostics, build your own transformer—that's schema-algebra territory,
// beyond JsonFusion's core scope.
template<class... Ts>
struct VariantOneOf {
    static_assert(
        (JsonFusion::static_schema::ParsableValue<Ts> && ...) &&
        (JsonFusion::static_schema::SerializableValue<Ts> && ...)
        ,
        "All variant types should be JsonFusion compatible");
    using wire_type = WireSink<32768>; // 32KB default buffer for variant parsing

    std::variant<Ts...> value;

    constexpr bool transform_from(const auto & parseFn) {
        std::size_t matched_counter = 0;
        std::variant<Ts...> temp;
        auto try_one = [&](auto c) {
            bool matched = !!parseFn(temp.template emplace<c.value>());
            if(matched) {
                matched_counter ++;
                if(matched_counter == 1) {
                    value = temp;
                }
            }
        };
        [&]<std::size_t ... Is>(std::index_sequence<Is...>){
            (try_one(std::integral_constant<std::size_t, Is>{}), ...);
        }(std::make_index_sequence<sizeof...(Ts)>{});

        if(matched_counter == 1) {
            return true;
        } else {
            return false;
        }
    }

    constexpr bool transform_to(const auto & serializeFn) const {
        return  std::visit([&](const auto& v) {
            return !!serializeFn(v);
        }, value);
    }
};

}
}
