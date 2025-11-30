#pragma once
#include "static_schema.hpp"
#include "struct_introspection.hpp"
#include "options.hpp"


namespace JsonFusion {
namespace json_path {
using namespace static_schema;
constexpr std::size_t SCHEMA_UNBOUNDED = std::numeric_limits<std::size_t>::max();
template <JsonParsableValue Type, class ... SeenTypes>
consteval std::size_t calc_type_depth() {
    using T = AnnotatedValue<Type>;

    if constexpr ( (std::is_same_v<T, SeenTypes> || ...) ) {
        return SCHEMA_UNBOUNDED;
    } else {
        if constexpr (JsonNullableParsableValue<T>) {
            if constexpr (is_specialization_of<T, std::optional>::value) {
                return calc_type_depth<typename T::value_type, SeenTypes...>();
            } else if constexpr (is_specialization_of<T, std::unique_ptr>::value) {
                return calc_type_depth<typename T::element_type, SeenTypes...>();
            } else {
                static_assert(false, "Bad nullable storage");
                return SCHEMA_UNBOUNDED;
            }
        } else {
            if constexpr (JsonBool<T> || JsonString<T> || JsonNumber<T>){
                return 1;
            } else { //containers
                if constexpr(ArrayWritable<T>) {
                    using ElemT = AnnotatedValue<typename array_write_cursor<AnnotatedValue<T>>::element_type>;
                    if(std::size_t r = calc_type_depth<ElemT, T, SeenTypes...>(); r == SCHEMA_UNBOUNDED) {
                        return SCHEMA_UNBOUNDED;
                    } else
                        return 1 + r;
                } else if constexpr(MapWritable<T>) {
                    using ElemT = AnnotatedValue<typename map_write_cursor<AnnotatedValue<T>>::mapped_type>;
                    if(std::size_t r = calc_type_depth<ElemT, T, SeenTypes...>(); r == SCHEMA_UNBOUNDED) {
                        return SCHEMA_UNBOUNDED;
                    } else
                        return 1 + r;
                } else { // objects

                    auto fieldDepthGetter = [](auto ic) -> std::size_t {
                        constexpr std::size_t StructIndex = decltype(ic)::value;
                        using Field   = introspection::structureElementTypeByIndex<StructIndex, T>;
                        using Opts    = options::detail::annotation_meta_getter<Field>::options;
                        if constexpr (Opts::template has_option<options::detail::not_json_tag>) {
                            return 0;
                        } else {
                            using FeildT = AnnotatedValue<Field>;
                            return calc_type_depth<FeildT, T, SeenTypes...>();
                        }
                    };

                    std::size_t r = [&]<std::size_t... I>(std::index_sequence<I...>) -> std::size_t {
                        return std::max({fieldDepthGetter(std::integral_constant<std::size_t, I>{})...});
                    }(std::make_index_sequence<introspection::structureElementsCount<T>>{});
                    if(r == SCHEMA_UNBOUNDED) {
                        return r;
                    } else {
                        return 1 +r;
                    }

                }
            }
        }
    }

}


struct PathElement {
    std::size_t     array_index = std::numeric_limits<std::size_t>::max();   // all array-like
    std::string_view field_name;  // consteval name for structs, key view for maps
};

template<std::size_t N>
struct JsonStaticPath {

    std::size_t currentLength = 0;

    std::array<PathElement, N> storage;
    constexpr JsonStaticPath () {
        currentLength = 1;
    }
    constexpr void push_child(PathElement && el) {
        storage[currentLength] = el;
        currentLength ++;
    }
    constexpr void pop() {
        currentLength --;
    }

};


struct JsonDynamicPath {

    std::vector<PathElement> storage;

    constexpr JsonDynamicPath () {
        storage.push_back(PathElement{});
    }
    constexpr void push_child(PathElement && el) {
        storage.emplace_back(std::forward<PathElement>(el));
    }
    constexpr void pop() {
       storage.pop_back();
    }

};


}
}
