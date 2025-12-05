#pragma once

#include <cstddef>
#include <string_view>
#include <pfr.hpp>


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


constexpr std::size_t NOT_A_MEMBER = static_cast<std::size_t>(-1);
template<class T, auto MemberPtr, std::size_t... Is>
consteval std::size_t index_for_member_ptr_impl(std::index_sequence<Is...>) {
    T obj{};                      // requires aggregate / trivial enough for PFR
    auto& target = obj.*MemberPtr;
    std::size_t result = NOT_A_MEMBER;

    // Fold-expr over all PFR indices
    (
        [&]{
            if (&target == &getStructElementByIndex<Is>(obj)) {
                result = Is;
            }
        }(),
        ...
        );

    return result;
}

template<class T, auto MemberPtr>
consteval std::size_t index_for_member_ptr() {
    constexpr std::size_t n = structureElementsCount<T>;
    return index_for_member_ptr_impl<T, MemberPtr>(std::make_index_sequence<n>{});
}


}
}
