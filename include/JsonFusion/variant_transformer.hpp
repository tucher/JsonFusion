#pragma once
#include <variant>

#include <JsonFusion/generic_transformers.hpp>
#include <JsonFusion/options.hpp>
#include <JsonFusion/static_schema.hpp>
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>


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

    constexpr bool transform_from(const wire_type& sink) {
        std::size_t matched_counter = 0;
        std::variant<Ts...> temp;
        auto try_one = [&](auto c) {
            bool matched = !!Parse(temp.template emplace<c.value>(), sink);
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

    constexpr bool transform_to(wire_type& wire) const {
        return  std::visit([&wire](const auto& v) {
            return !!Serialize(v, wire);
        }, value);
    }
};

}
}
