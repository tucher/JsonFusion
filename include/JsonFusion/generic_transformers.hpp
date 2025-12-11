#pragma once
#include <JsonFusion/static_schema.hpp>
namespace JsonFusion {
namespace transformers {
    
template<
    class StoredT,
    class WireT,
    auto FromFn,   // bool(StoredT&, const WireT&)
    auto ToFn      // bool(const StoredT&, WireT&)
>
struct Transformed {
    using stored_type = StoredT;
    using wire_type   = WireT;

    StoredT value{};  // what the user “really” owns in the model

    

     // ---- Parse side: wire -> stored ----
    constexpr bool transform_from(const WireT& wire) {
        return FromFn(value, wire);
    }

    // ---- Serialize side: stored -> wire ----
    constexpr bool transform_to(WireT& wire) const {
        return ToFn(value, wire);
    }

    constexpr Transformed() = default;

    constexpr Transformed(const Transformed&) = default;
    constexpr Transformed(Transformed&&) = default;
    constexpr Transformed& operator=(const Transformed&) = default;
    constexpr Transformed& operator=(Transformed&&) = default;

    // construct from StoredT or anything convertible to StoredT
    template<class U>
        requires std::convertible_to<U, StoredT>
    constexpr Transformed(U&& u) : value(std::forward<U>(u)) {}

    template<class U>
        requires std::convertible_to<U, StoredT>
    constexpr Transformed& operator=(U&& u) {
        value = std::forward<U>(u);
        return *this;
    }

    constexpr operator StoredT&()             { return value; }
    constexpr operator const StoredT&() const { return value; }

    constexpr Transformed& operator=(const StoredT& v) {
        value = v;
        return *this;
    }
    constexpr StoredT*       operator->()       { return std::addressof(value); }
    constexpr const StoredT* operator->() const { return std::addressof(value); }

    constexpr StoredT&       get()       { return value; }
    constexpr const StoredT& get() const { return value; }


    template<class U = StoredT>
        requires std::ranges::range<U>
    constexpr auto begin() {
        using std::begin;
        return begin(value);
    }

    template<class U = StoredT>
        requires std::ranges::range<const U>
    constexpr auto begin() const {
        using std::begin;
        return begin(value);
    }

    template<class U = StoredT>
        requires std::ranges::range<U>
    constexpr auto end() {
        using std::end;
        return end(value);
    }

    template<class U = StoredT>
        requires std::ranges::range<const U>
    constexpr auto end() const {
        using std::end;
        return end(value);
    }

    // size() forwarding (string, vector, array, etc.)
    template<class U = StoredT>
        requires requires (const U& u) { u.size(); }
    constexpr auto size() const {
        return value.size();
    }

    // data() forwarding
    template<class U = StoredT>
        requires requires (U& u) { u.data(); }
    constexpr auto data() {
        return value.data();
    }

    template<class U = StoredT>
        requires requires (const U& u) { u.data(); }
    constexpr auto data() const {
        return value.data();
    }

    // operator[] forwarding (arrays, string, vector, etc.)
    template<class U = StoredT>
        requires requires (U& u) { u[std::size_t{0}]; }
    constexpr decltype(auto) operator[](std::size_t i) {
        return value[i];
    }

    template<class U = StoredT>
        requires requires (const U& u) { u[std::size_t{0}]; }
    constexpr decltype(auto) operator[](std::size_t i) const {
        return value[i];
    }

    // push_back (strings, vectors, etc.)
    template<class U = StoredT>
        requires requires (U& u, const typename U::value_type& x) { u.push_back(x); }
    constexpr void push_back(const typename U::value_type& x) {
        value.push_back(x);
    }

    // clear()
    template<class U = StoredT>
        requires requires (U& u) { u.clear(); }
    constexpr void clear() {
        value.clear();
    }

   
};


template<class T, class... OptsL, class... OptsR>
constexpr bool operator==(const Transformed<T, OptsL...>& lhs,
                const Transformed<T, OptsR...>& rhs)
    noexcept(noexcept(lhs.value == rhs.value))
{
    return lhs.value == rhs.value;
}

template<class T, class... OptsL, class... OptsR>
constexpr bool operator!=(const Transformed<T, OptsL...>& lhs,
                const Transformed<T, OptsR...>& rhs)
    noexcept(noexcept(lhs.value != rhs.value))
{
    return lhs.value != rhs.value;
}

// 2) Transformed<T, ...> == U (for any U where T == U is valid)
template<class T, class... Opts, class U>
    requires requires (const T& t, const U& u) { t == u; }
constexpr bool operator==(const Transformed<T, Opts...>& lhs,
                const U& rhs)
    noexcept(noexcept(std::declval<const T&>() == std::declval<const U&>()))
{
    return lhs.value == rhs;
}

template<class T, class... Opts, class U>
    requires requires (const T& t, const U& u) { t != u; }
constexpr bool operator!=(const Transformed<T, Opts...>& lhs,
                const U& rhs)
    noexcept(noexcept(std::declval<const T&>() != std::declval<const U&>()))
{
    return lhs.value != rhs;
}

// 3) U == Transformed<T, ...>
template<class U, class T, class... Opts>
    requires requires (const U& u, const T& t) { u == t; }
constexpr bool operator==(const U& lhs,
                const Transformed<T, Opts...>& rhs)
    noexcept(noexcept(std::declval<const U&>() == std::declval<const T&>()))
{
    return lhs == rhs.value;
}

template<class U, class T, class... Opts>
    requires requires (const U& u, const T& t) { u != t; }
constexpr bool operator!=(const U& lhs,
                const Transformed<T, Opts...>& rhs)
    noexcept(noexcept(std::declval<const U&>() != std::declval<const T&>()))
{
    return lhs != rhs.value;
}


template<class ElemWire, class StoredT, auto ReduceFn>
struct ArrayReduceField {
    struct Streamer {
        using value_type = ElemWire;
        StoredT state{};

        void reset() { state = StoredT{}; }
        bool consume(const value_type& v) {
            return std::invoke(ReduceFn, state, v);
        }
        bool finalize(bool ok) { return ok; }
    };

    using wire_type = Streamer;
    StoredT value{};

    bool transform_from(const wire_type& w) {
        value = w.state;
        return true;
    }
};

}
}
