#pragma once

#include <array>
#include <cstdint>
#include <iterator>
#include <string_view>
#include <utility>  // std::declval
#include <ranges>
#include <type_traits>
#include <pfr.hpp>
#include <algorithm>

#include "options.hpp"

namespace JsonFusion {
namespace introspection {


struct FieldDescr {
    std::string_view name;
    std::size_t originalIndex;

};

template<class T>
struct FieldsHelper {
    template<std::size_t I>
    static consteval bool fieldIsNotJSON() {
        using Field   = pfr::tuple_element_t<I, T>;
        using Opts    = options::detail::annotation_meta_getter<Field>::options;
        if constexpr (Opts::template has_option<options::detail::not_json_tag>) {
            return true;
        } else {
            return false;
        }
    }

    static constexpr std::size_t rawFieldsCount = pfr::tuple_size_v<T>;

    static constexpr std::size_t fieldsCount = []<std::size_t... I>(std::index_sequence<I...>) consteval{
        //TODO get name from options, if presented
        return (std::size_t{0} + ... + (!fieldIsNotJSON<I>() ? 1: 0));
    }(std::make_index_sequence<rawFieldsCount>{});



    template<std::size_t I>
    static consteval std::string_view fieldName() {
        using Field   = pfr::tuple_element_t<I, T>;
        using Opts    = options::detail::annotation_meta_getter<Field>::options;

        if constexpr (Opts::template has_option<options::detail::key_tag>) {
            using KeyOpt = typename Opts::template get_option<options::detail::key_tag>;
            return KeyOpt::desc.toStringView();
        } else {
            return pfr::get_name<I, T>();
        }
    }

    static constexpr std::array<FieldDescr, fieldsCount> fieldIndexesSortedByFieldName =
        []<std::size_t... I>(std::index_sequence<I...>) consteval {
            std::array<FieldDescr, fieldsCount> arr;
            std::size_t index = 0;
            auto add_one = [&](auto ic) consteval {
                constexpr std::size_t J = decltype(ic)::value;
                if constexpr (!fieldIsNotJSON<J>()) {
                    arr[index++] = FieldDescr{ fieldName<J>(), J };
                }
            };
            (add_one(std::integral_constant<std::size_t, I>{}), ...);
            std::ranges::sort(arr, {}, &FieldDescr::name);

            return arr;
        }(std::make_index_sequence<rawFieldsCount>{});

    static constexpr bool fieldsAreUnique = [](std::array<FieldDescr, fieldsCount> sortedArr) consteval{
        return std::ranges::adjacent_find(sortedArr, {}, &FieldDescr::name) == sortedArr.end();
    }(fieldIndexesSortedByFieldName);


    static consteval std::size_t indexInSortedByName(std::string_view name) {
        for(int i = 0; i < fieldsCount; i++) {
            if(fieldIndexesSortedByFieldName[i].name == name) {
                return i;
            }
        }
        return -1;
    }
};

}

}
