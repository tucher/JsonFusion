# JsonFusion

Zero-boilerplate, constexpr-capable JSON/CBOR/YAML serialization for C++23.
Just define your structs — JsonFusion handles the rest using compile-time reflection.

## Documentation

| I want to... | Go to... |
|---|---|
| Learn step by step | [Tutorials](tutorials/01-getting-started.md) |
| Solve a specific problem | How-to Guides *(coming soon)* |
| Look up API details | Reference *(coming soon)* |
| Understand design decisions | [Why Reflection?](WHY_REFLECTION.md) |

## Quick Example

```cpp
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>

struct Network {
    std::string                name;
    std::string                address;
    int port;
};

int main() {
    Network conf;

    JsonFusion::Parse(conf, R"({"name":"1","address":"0.0.0.0","port":8081})");

    std::string output;
    JsonFusion::Serialize(conf, output);
    return 0;
}
```

No macros. No boilerplate. No code generation. Just C++ structs.

## Try it live

Every code block marked as interactive can be compiled 
directly in the browser — powered by Clang compiled to WebAssembly.

```cpp live check-only
#include <JsonFusion/parser.hpp>

static_assert([]() constexpr {
    struct Point { int x{}; int y{}; };
    Point conf;
    JsonFusion::Parse(conf, R"({"x":10,"y":20})");
    return conf.x == 10 && conf.y == 20;
}());
```

## More

- [Run the full constexpr test suite](constexpr-tests.html) — all tests execute in your browser
- [GitHub repository](https://github.com/tucher/JsonFusion)
