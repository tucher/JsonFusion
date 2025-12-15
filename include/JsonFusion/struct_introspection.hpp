#pragma once

#include <cstddef>
#include <string_view>
#include <pfr/tuple_size.hpp>
#include <pfr/core.hpp>
#include <pfr/core_name.hpp>

#include "const_string.hpp"
#include "annotated.hpp"

namespace JsonFusion {

template <class ... Flds>
struct StructMeta {

};
template <auto MPtr, ConstString key, class ... Opts>
struct Field;

template <typename C, typename T, T C::*MPtr, ConstString key, class ... Opts>
struct Field<MPtr, key, Opts...>{
    using ClassT = C;
    using ValueT = T;
    using OptionsP = OptionsPack<Opts...>;
    static constexpr ConstString Name  = key;
    static constexpr  T C::* MemberP = MPtr;

};



template <class ... F>
struct StructFields{
    using FieldsTuple = std::tuple<F...>;
};


namespace introspection {

namespace detail {


template<class T>
struct IntrospectionImpl {
    using StructT = std::remove_cv_t<T>;

    template<std::size_t Index>
    static constexpr decltype(auto) getStructElementByIndex(StructT & s) {
        return (pfr::get<Index>(s));
    }

    template<std::size_t Index>
        static constexpr decltype(auto) getStructElementByIndex(const StructT & s) {
        return (pfr::get<Index>(s));
    }

    static constexpr std::size_t structureElementsCount = pfr::tuple_size_v<StructT>;

    template<std::size_t Index>
    using structureElementTypeByIndex = pfr::tuple_element_t<Index, StructT>;

    template<std::size_t Index>
    static constexpr std::string_view structureElementNameByIndex = pfr::get_name<Index, StructT>();
};

template<class T>
struct is_fields_pack : std::false_type {};

template<class... Field>
struct is_fields_pack<StructFields<Field...>> : std::true_type {};

template<class T>
inline constexpr bool is_fields_pack_v = is_fields_pack<T>::value;


template<class T, class = void>
struct has_struct_meta_specialization_impl : std::false_type {};

template<class T>
struct has_struct_meta_specialization_impl<T,
                                          std::void_t<typename StructMeta<T>::Fields>
                                          > : std::bool_constant<
                                                  is_fields_pack_v<typename StructMeta<T>::Fields>

                                                  > {};

template<class T>
inline constexpr bool has_struct_meta_specialization =
    has_struct_meta_specialization_impl<T>::value;

template <class T, class OptPack> struct AnnotationFiller;
template <class T, class ...Opts> struct AnnotationFiller<T, OptionsPack<Opts...>> {
    using type = Annotated<T, Opts...>;
};


template <class T>
requires (has_struct_meta_specialization<T>)
struct IntrospectionImpl<T> {
    using Fields = typename StructMeta<T>::Fields::FieldsTuple;
    static constexpr std::size_t structureElementsCount = std::tuple_size_v<Fields>;


    template<std::size_t Index, class StructT>
    static constexpr decltype(auto) getStructElementByIndex(StructT & s) {
        using Field = std::tuple_element_t<Index, Fields>;
        return (s.*(Field::MemberP));
    }

    template<std::size_t Index, class StructT>
    static constexpr decltype(auto) getStructElementByIndex(const StructT & s) {
        using Field = std::tuple_element_t<Index, Fields>;
        return (s.*(Field::MemberP));
    }


    template<std::size_t Index>
    using structureElementTypeByIndex = AnnotationFiller<
                                        typename std::tuple_element_t<Index, Fields>::ValueT,
                                        typename std::tuple_element_t<Index, Fields>::OptionsP
                                        >::type;


    template<std::size_t Index>
    static constexpr std::string_view structureElementNameByIndex =
        std::tuple_element_t<Index, Fields>::Name.toStringView();
};



}
template<std::size_t Index, class StructT>
constexpr decltype(auto) getStructElementByIndex(StructT & s) {
    using Impl = detail::IntrospectionImpl<std::remove_cv_t<StructT>>;
    return (Impl::template getStructElementByIndex<Index>(s));
}

template<std::size_t Index, class StructT>
constexpr decltype(auto) getStructElementByIndex(const StructT & s) {
    using Impl = detail::IntrospectionImpl<std::remove_cv_t<StructT>>;
    return (Impl::template getStructElementByIndex<Index>(
        const_cast<std::remove_cv_t<StructT>&>(s)
        ));
    // return (detail::IntrospectionImpl<StructT>::template getStructElementByIndex<Index>(s));
}
template<class StructT>
static constexpr std::size_t structureElementsCount = detail::IntrospectionImpl<std::remove_cv_t<StructT>>::structureElementsCount;

template<std::size_t Index, class StructT>
using structureElementTypeByIndex = detail::IntrospectionImpl<std::remove_cv_t<StructT>>::template structureElementTypeByIndex<Index>;

template<std::size_t Index, class StructT>
static constexpr std::string_view structureElementNameByIndex = detail::IntrospectionImpl<std::remove_cv_t<StructT>>::template structureElementNameByIndex<Index>;




namespace detail {

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
}
