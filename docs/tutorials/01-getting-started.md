# Getting Started

This tutorial takes you from zero to a working JSON parse in a few steps.
Every code block below is interactive — click **Compile** to run it in your browser.

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

constexpr auto data = R"({"port": "string val", "host": "localhost"})";

static_assert([]() constexpr {
    Config conf;
    return !!JsonFusion::Parse(conf, data);
}());

```

## Nested Structures

Structs can contain other structs. JsonFusion handles nesting automatically:

```cpp live check-only
#include <JsonFusion/json.hpp>

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

## Serialization

Converting back to JSON is just as simple:

```cpp live check-only
#include <JsonFusion/json.hpp>

struct Point { int x; int y; };

```

## Validation with static_assert

Because parsing is `constexpr`, you can validate your JSON schemas at compile time.
If the assertion fails, you'll see a compilation error — not a runtime crash:

```cpp live check-only
#include <JsonFusion/json.hpp>

struct Config {
    int port;
    std::string host;
};

```

> Try changing the assertion to `cfg.port > 9000` — what happens?

## What's Next

- Learn about [validation and constraints](02-basic-validation.md) *(coming soon)*
- Explore the [type mapping reference](../reference/api/parse.md) *(coming soon)*
- Read [Why Reflection?](../WHY_REFLECTION.md) to understand the design

[Back to Documentation](../index.md)
