#pragma once

#include "string_search.hpp"
namespace JsonFusion {

namespace struct_fields_helper {
template<class T, std::size_t I>
static consteval bool fieldIsNotJSON() {
    using Opts    = options::detail::aggregate_field_opts_getter<T, I>;
    if constexpr (Opts::template has_option<options::detail::not_json_tag>) {
        return true;
    } else {
        return false;
    }
}


template<class T>
struct FieldsHelper {
    using FieldDescr = string_search::StringDescr;


    static constexpr std::size_t rawFieldsCount = introspection::structureElementsCount<T>;

    static constexpr std::size_t fieldsCount = []<std::size_t... I>(std::index_sequence<I...>) consteval{
        return (std::size_t{0} + ... + (!fieldIsNotJSON<T, I>() ? 1: 0));
    }(std::make_index_sequence<rawFieldsCount>{});



    template<std::size_t I>
    static consteval std::string_view fieldName() {
        using Opts    = options::detail::aggregate_field_opts_getter<T, I>;
        if constexpr (Opts::template has_option<options::detail::key_tag>) {
            using KeyOpt = typename Opts::template get_option<options::detail::key_tag>;
            return KeyOpt::desc.toStringView();
        } else {
            return introspection::structureElementNameByIndex<I, T>;
        }
    }

    static constexpr std::array<FieldDescr, fieldsCount> fieldIndexesToFieldNames =
        []<std::size_t... I>(std::index_sequence<I...>) consteval {
            std::array<FieldDescr, fieldsCount> arr;
            std::size_t index = 0;
            auto add_one = [&](auto ic) consteval {
                constexpr std::size_t J = decltype(ic)::value;
                if constexpr (!fieldIsNotJSON<T, J>()) {
                    arr[index++] = FieldDescr{ fieldName<J>(), J };
                }
            };
            (add_one(std::integral_constant<std::size_t, I>{}), ...);
            return arr;
        }(std::make_index_sequence<rawFieldsCount>{});

    template<std::size_t StructIndex>
    static consteval std::size_t fieldIndexKey() {



        std::size_t ret = 0;
        [&]<std::size_t... I>(std::index_sequence<I...>) consteval {

            std::size_t i = 0;
            auto get_one = [&](auto ic)  consteval  {
                constexpr std::size_t J = decltype(ic)::value;
                using Opts    = options::detail::aggregate_field_opts_getter<T, J>;
                if constexpr (J == StructIndex) {
                    if constexpr (Opts::template has_option<options::detail::numeric_key_tag>) {
                        using KeyOpt = typename Opts::template get_option<options::detail::numeric_key_tag>;
                        ret = KeyOpt::NumericKey;
                    } else {
                        ret = i;
                    }
                }
                if constexpr (!fieldIsNotJSON<T, J>()) {
                    i ++;
                }
            };

            (get_one(std::integral_constant<std::size_t, I>{}),...);

        }(std::make_index_sequence<StructIndex+1>{});
        return ret;
    }
    static constexpr std::size_t maxIndexKeyVal = []<std::size_t... I>(std::index_sequence<I...>) consteval {
        if constexpr(sizeof...(I) == 0) {
            return 0;
        } else
            return std::max({fieldIndexKey<I>()...});
    }(std::make_index_sequence<rawFieldsCount>{});

    using field_index_t =
        std::conditional_t<(maxIndexKeyVal <= 255), std::uint8_t,
                           std::conditional_t<(maxIndexKeyVal <= 65535), std::uint16_t,
                                              std::size_t>>;

    using field_raw_index_t =
        std::conditional_t<(rawFieldsCount <= 255), std::uint8_t,
                           std::conditional_t<(rawFieldsCount <= 65535), std::uint16_t,
                                              std::size_t>>;

    static constexpr std::array<std::pair<field_index_t, field_raw_index_t>, fieldsCount> fieldIndexes =
        []<std::size_t... I>(std::index_sequence<I...>) consteval {
            std::array<std::pair<field_index_t, field_raw_index_t>, fieldsCount> arr;

            std::size_t i = 0;
            auto get_one = [&](auto ic)  consteval  {
                constexpr std::size_t J = decltype(ic)::value;
                if constexpr (fieldIsNotJSON<T, J>()) {
                    return;
                } else {
                    arr[i] = std::pair<field_index_t, field_raw_index_t>{fieldIndexKey<J>(), J};
                    i ++;
                }


            };

            (get_one(std::integral_constant<std::size_t, I>{}),...);



            return arr;
        }(std::make_index_sequence<rawFieldsCount>{});

    static constexpr bool fieldsAreUnique = [](std::array<FieldDescr, fieldsCount> inputArr) consteval{
        auto sortedArr = inputArr;
        std::ranges::sort(sortedArr, {}, &FieldDescr::name);
        return std::ranges::adjacent_find(sortedArr, {}, &FieldDescr::name) == sortedArr.end();
    }(fieldIndexesToFieldNames) && [](std::array<std::pair<field_index_t, field_raw_index_t>, fieldsCount> inputArr) consteval{
        auto sortedArr = inputArr;
        std::ranges::sort(sortedArr, {}, &std::pair<field_index_t, field_raw_index_t>::first);
        return std::ranges::adjacent_find(sortedArr, {}, &std::pair<field_index_t, field_raw_index_t>::first) == sortedArr.end();
    }(fieldIndexes);

    static constexpr std::size_t maxFieldNameLength = []() consteval {
        std::size_t maxLen = 0;
        for (const auto& field : fieldIndexesToFieldNames) {
            if (field.name.size() > maxLen) {
                maxLen = field.name.size();
            }
        }
        return maxLen;
    }();

    static consteval std::size_t indexInSortedByName(std::string_view name) {
        for(std::size_t i = 0; i < fieldsCount; i++) {
            if(fieldIndexesToFieldNames[i].name == name) {
                return i;
            }
        }
        return -1;
    }


    // static constexpr string_search::PerfectHashDFA<fieldsCount, maxFieldNameLength> dfa = []() consteval {
    //     return string_search::PerfectHashDFA<fieldsCount, maxFieldNameLength>(fieldIndexesToFieldNames);
    // }();
};
}
}
