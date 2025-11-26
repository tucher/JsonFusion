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
template<std::size_t Index, class StructT>
constexpr decltype(auto) getStructElementByIndex(StructT & s) {
    return (pfr::get<Index>(s));
}

template<std::size_t Index, class StructT>
constexpr decltype(auto) getStructElementByIndex(const StructT & s) {
    return (pfr::get<Index>(s));
}
template<class StructT>
static constexpr std::size_t structureElementsCount = pfr::tuple_size_v<StructT>;

template<std::size_t Index, class StructT>
using structureElementTypeByIndex = pfr::tuple_element_t<Index, StructT>;

template<std::size_t Index, class StructT>
static constexpr std::string_view structureElementNameByIndex = pfr::get_name<Index, StructT>();





}
}
