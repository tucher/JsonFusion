#pragma once
#include <JsonFusion/static_schema.hpp>
namespace JsonFusion {
namespace streamers {
    
template <class ValueT>
struct CountingStreamer {
    using value_type = ValueT;

    std::size_t counter = 0;

    void reset()  {counter = 0;}

    // Called for each element, with a fully-parsed value_type
    bool consume(const ValueT & )  {
        counter ++;
        return true;
    }
    bool finalize(bool success)  { return true; }
};


template<auto Fn>
struct LambdaStreamer;

namespace detail{
// Helper: turn a function pointer type bool(Ctx*, const Arg&) into traits
template<typename>
struct lambda_streamer_traits;

// Supported callable shape: bool(*)(Ctx*, const Arg&)
// Warning: be really careful with inlining: mark the function as noinline if it is not a small function
template<typename Ctx, typename Arg>
struct lambda_streamer_traits<bool(*)(Ctx*, const Arg&)> {
    using ctx_type   = Ctx;
    using value_type = std::remove_cvref_t<Arg>;
};

// Fallback: if signature doesn't match, trigger a readable static_assert
template<typename T>
struct lambda_streamer_traits {
    static_assert(sizeof(T) == 0,
                  "LambdaStreamer: Fn must be callable as bool(Ctx*, const Value&)");
};

}
template<auto Fn>
struct LambdaStreamer {
    // Force conversion of lambdas to function pointer to inspect their signature.
    using fn_ptr   = decltype(+Fn);
    using traits   = detail::lambda_streamer_traits<fn_ptr>;

    using ctx_type   = typename traits::ctx_type;
    using value_type = typename traits::value_type;

    ctx_type* ctx = nullptr;


    constexpr void reset() noexcept {
        // No internal buffering by default; user can reset ctx externally if needed.
    }

    constexpr bool consume(const value_type& v) noexcept {
        // Call user callable with (ctx, value)
        return Fn(ctx, v);
    }

    constexpr bool finalize(bool success) noexcept {
        // You can make this call Fn(ctx, ...) again if you ever want a "flush" hook,
        // but simple passthrough is usually enough.
        return success;
    }

    // Called by JsonFusion to inject the context pointer when building the model.
    constexpr void set_jsonfusion_context(ctx_type* c) noexcept {
        ctx = c;
    }
};



}
}
