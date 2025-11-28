#pragma once

#include <iterator>
#include <ranges>
#include <utility>
#include <cstddef>

namespace JsonFusion {


template <class T, typename... Options>
struct Annotated {
    T value{};
    using value_type = T;
    // Conversions/forwarding so parser can treat Annotated<T> like T


    // defaulted special members
    constexpr Annotated() = default;
    constexpr Annotated(const Annotated&) = default;
    constexpr Annotated(Annotated&&) = default;
    constexpr Annotated& operator=(const Annotated&) = default;
    constexpr Annotated& operator=(Annotated&&) = default;

    // construct from T or anything convertible to T
    template<class U>
        requires std::convertible_to<U, T>
    constexpr Annotated(U&& u) : value(std::forward<U>(u)) {}

    template<class U>
        requires std::convertible_to<U, T>
    constexpr Annotated& operator=(U&& u) {
        value = std::forward<U>(u);
        return *this;
    }

    constexpr operator T&()             { return value; }
    constexpr operator const T&() const { return value; }

    constexpr Annotated& operator=(const T& v) {
        value = v;
        return *this;
    }
    constexpr T*       operator->()       { return std::addressof(value); }
    constexpr const T* operator->() const { return std::addressof(value); }

    constexpr T&       get()       { return value; }
    constexpr const T& get() const { return value; }


    template<class U = T>
        requires std::ranges::range<U>
    constexpr auto begin() {
        using std::begin;
        return begin(value);
    }

    template<class U = T>
        requires std::ranges::range<const U>
    constexpr auto begin() const {
        using std::begin;
        return begin(value);
    }

    template<class U = T>
        requires std::ranges::range<U>
    constexpr auto end() {
        using std::end;
        return end(value);
    }

    template<class U = T>
        requires std::ranges::range<const U>
    constexpr auto end() const {
        using std::end;
        return end(value);
    }

    // size() forwarding (string, vector, array, etc.)
    template<class U = T>
        requires requires (const U& u) { u.size(); }
    constexpr auto size() const {
        return value.size();
    }

    // data() forwarding
    template<class U = T>
        requires requires (U& u) { u.data(); }
    constexpr auto data() {
        return value.data();
    }

    template<class U = T>
        requires requires (const U& u) { u.data(); }
    constexpr auto data() const {
        return value.data();
    }

    // operator[] forwarding (arrays, string, vector, etc.)
    template<class U = T>
        requires requires (U& u) { u[std::size_t{0}]; }
    constexpr decltype(auto) operator[](std::size_t i) {
        return value[i];
    }

    template<class U = T>
        requires requires (const U& u) { u[std::size_t{0}]; }
    constexpr decltype(auto) operator[](std::size_t i) const {
        return value[i];
    }

    // push_back (strings, vectors, etc.)
    template<class U = T>
        requires requires (U& u, const typename U::value_type& x) { u.push_back(x); }
    constexpr void push_back(const typename U::value_type& x) {
        value.push_back(x);
    }

    // clear()
    template<class U = T>
        requires requires (U& u) { u.clear(); }
    constexpr void clear() {
        value.clear();
    }
};
// 1) Annotated<T, ...> == Annotated<T, ...>
template<class T, class... OptsL, class... OptsR>
constexpr bool operator==(const Annotated<T, OptsL...>& lhs,
                const Annotated<T, OptsR...>& rhs)
    noexcept(noexcept(lhs.value == rhs.value))
{
    return lhs.value == rhs.value;
}

template<class T, class... OptsL, class... OptsR>
constexpr bool operator!=(const Annotated<T, OptsL...>& lhs,
                const Annotated<T, OptsR...>& rhs)
    noexcept(noexcept(lhs.value != rhs.value))
{
    return lhs.value != rhs.value;
}

// 2) Annotated<T, ...> == U (for any U where T == U is valid)
template<class T, class... Opts, class U>
    requires requires (const T& t, const U& u) { t == u; }
constexpr bool operator==(const Annotated<T, Opts...>& lhs,
                const U& rhs)
    noexcept(noexcept(std::declval<const T&>() == std::declval<const U&>()))
{
    return lhs.value == rhs;
}

template<class T, class... Opts, class U>
    requires requires (const T& t, const U& u) { t != u; }
constexpr bool operator!=(const Annotated<T, Opts...>& lhs,
                const U& rhs)
    noexcept(noexcept(std::declval<const T&>() != std::declval<const U&>()))
{
    return lhs.value != rhs;
}

// 3) U == Annotated<T, ...>
template<class U, class T, class... Opts>
    requires requires (const U& u, const T& t) { u == t; }
constexpr bool operator==(const U& lhs,
                const Annotated<T, Opts...>& rhs)
    noexcept(noexcept(std::declval<const U&>() == std::declval<const T&>()))
{
    return lhs == rhs.value;
}

template<class U, class T, class... Opts>
    requires requires (const U& u, const T& t) { u != t; }
constexpr bool operator!=(const U& lhs,
                const Annotated<T, Opts...>& rhs)
    noexcept(noexcept(std::declval<const U&>() != std::declval<const T&>()))
{
    return lhs != rhs.value;
}

} // namespace JsonFusion
