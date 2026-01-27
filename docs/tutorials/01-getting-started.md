# Getting Started

This tutorial takes you from zero to a working JSON parse in a few steps.
Every code block below is interactive — click **Compile** to compile it right in your browser. Because JsonFusion is `constexpr`, you can use `static_assert` for tests instead of `assert`: testing/playing without running any binaries. If the assertion fails, you'll see a compilation error. 

## Your First Parse

Define a C++ struct and parse JSON into it. No macros, no registration — just a struct:

```cpp live check-only
#include <JsonFusion/parser.hpp>

struct Config {
    int port;
    std::string host;
};

constexpr auto data = R"({"port": 8080, "host": "localhost"})";

static_assert([]() constexpr {
    Config conf;
    return 
        !!JsonFusion::Parse(conf, data) 
        && conf.port == 8080 
        && conf.host == "localhost";
}());

```

The `Parse(object, data)` function returns a bool-convertible result. 

> Try changing `8080` to another value and recompile! This will fail:

```cpp live check-only
#include <JsonFusion/parser.hpp>

struct Config {
    int port;
    std::string host;
};

constexpr auto data = R"({"port": 8081, "host": "localhost"})";

constexpr auto res = []() constexpr {
    Config conf;
    return std::make_pair(!!JsonFusion::Parse(conf, data), conf);
}();

static_assert(res.first);
static_assert(res.second.port==8080);

```

## Serialization

Converting back to JSON is just as simple:

```cpp live check-only
#include <JsonFusion/serializer.hpp>
#include <vector>

struct Point { int x; int y; };

static_assert([]() constexpr {
    std::string res;
    JsonFusion::Serialize(
        std::vector<Point> {
            {1, 2}, {3, 4}
        }, res);
    return res == R"([{"x":1,"y":2},{"x":3,"y":4}])";
}());
```

## Nested Structures

Structs can contain other structs. JsonFusion handles nesting automatically:

```cpp live check-only

struct Address {
    std::string street;
    std::string city;
};

struct Person {
    std::string name;
    int age;
    Address address;
};

```

## Validation with static_assert

Because parsing is `constexpr`, you can validate your JSON schemas at compile time.
If the assertion fails, you'll see a compilation error — not a runtime crash:

```cpp live check-only

struct Config {
    int port;
    std::string host;
};

```

> Try changing the assertion to `cfg.port > 9000` — what happens?

## What's Next

- [Modeling JSON with C++ Types](02-modeling-json-types.md) — the six JSON types, containers, and composition
- [Validation & Annotations](03-validation-and-annotations.md) — constraints, `A<>` wrapper, external annotations
- [Error Handling](04-error-handling.md) — ParseResult, JSON paths, diagnostics
- [Streaming](05-streaming.md) — count canada.json primitives without loading the whole file into memory.
- Read [Why Reflection?](../WHY_REFLECTION.md) to understand the design

[Back to Documentation](../index.md)
