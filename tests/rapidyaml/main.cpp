#define RYML_SINGLE_HDR_DEFINE_NOW
#include "JsonFusion/yaml.hpp"
#include "JsonFusion/parser.hpp"
#include "JsonFusion/serializer.hpp"
#include <iostream>
#include <cassert>

using namespace JsonFusion;

// Simple test struct
struct Point {
    int x;
    int y;
};

struct Config {
    std::string name;
    int value;
    bool enabled;
    std::vector<int> items;
};

int main() {
    std::cout << "=== RapidYaml Reader Tests ===\n\n";

    // Test 1: Parse simple map
    {
        std::cout << "Test 1: Parse simple map... ";
        const char* yaml = R"(
x: 10
y: 20
)";
        RapidYamlReader reader(yaml, std::strlen(yaml));
        assert(reader.getError() == RapidYamlReader::ParseError::NO_ERROR);

        Point p{};
        auto result = ParseWithReader(p, reader);
        assert(result.error() == ParseError::NO_ERROR);
        assert(p.x == 10);
        assert(p.y == 20);
        std::cout << "PASSED (x=" << p.x << ", y=" << p.y << ")\n";
    }

    // Test 2: Parse array
    {
        std::cout << "Test 2: Parse array... ";
        const char* yaml = R"(
- 1
- 2
- 3
- 4
- 5
)";
        RapidYamlReader reader(yaml, std::strlen(yaml));
        assert(reader.getError() == RapidYamlReader::ParseError::NO_ERROR);

        std::vector<int> vec;
        auto result = ParseWithReader(vec, reader);
        assert(result.error() == ParseError::NO_ERROR);
        assert(vec.size() == 5);
        assert(vec[0] == 1);
        assert(vec[4] == 5);
        std::cout << "PASSED (size=" << vec.size() << ")\n";
    }

    // Test 3: Parse nested structure
    {
        std::cout << "Test 3: Parse nested structure... ";
        const char* yaml = R"(
name: "test config"
value: 42
enabled: true
items:
  - 10
  - 20
  - 30
)";
        RapidYamlReader reader(yaml, std::strlen(yaml));
        assert(reader.getError() == RapidYamlReader::ParseError::NO_ERROR);

        Config cfg{};
        auto result = ParseWithReader(cfg, reader);
        assert(result.error() == ParseError::NO_ERROR);
        assert(cfg.name == "test config");
        assert(cfg.value == 42);
        assert(cfg.enabled == true);
        assert(cfg.items.size() == 3);
        assert(cfg.items[0] == 10);
        std::cout << "PASSED (name=" << cfg.name << ", value=" << cfg.value << ")\n";
    }

    // Test 4: Parse flow style (JSON-like)
    {
        std::cout << "Test 4: Parse flow style... ";
        const char* yaml = R"({x: 100, y: 200})";
        RapidYamlReader reader(yaml, std::strlen(yaml));
        assert(reader.getError() == RapidYamlReader::ParseError::NO_ERROR);

        Point p{};
        auto result = ParseWithReader(p, reader);
        assert(result.error() == ParseError::NO_ERROR);
        assert(p.x == 100);
        assert(p.y == 200);
        std::cout << "PASSED (x=" << p.x << ", y=" << p.y << ")\n";
    }

    // Test 5: Reject anchors
    {
        std::cout << "Test 5: Reject anchors... ";
        const char* yaml = R"(
anchor: &myanchor
  x: 1
ref: *myanchor
)";
        RapidYamlReader reader(yaml, std::strlen(yaml));
        assert(reader.getError() == RapidYamlReader::ParseError::UNSUPPORTED_YAML_FEATURE);
        std::cout << "PASSED (correctly rejected)\n";
    }

    // Test 6: Booleans - only true/false accepted
    {
        std::cout << "Test 6: Boolean parsing... ";
        const char* yaml = R"(enabled: true)";
        RapidYamlReader reader(yaml, std::strlen(yaml));

        struct BoolTest { bool enabled; };
        BoolTest bt{};
        auto result = ParseWithReader(bt, reader);
        assert(result.error() == ParseError::NO_ERROR);
        assert(bt.enabled == true);
        std::cout << "PASSED\n";
    }

    // Test 7: Null values
    {
        std::cout << "Test 7: Null values... ";
        const char* yaml = R"(value: null)";
        RapidYamlReader reader(yaml, std::strlen(yaml));

        struct NullTest { std::optional<int> value; };
        NullTest nt{};
        auto result = ParseWithReader(nt, reader);
        assert(result.error() == ParseError::NO_ERROR);
        assert(!nt.value.has_value());
        std::cout << "PASSED\n";
    }

    std::cout << "\n=== RapidYaml Writer Tests ===\n\n";

    // Test 8: Serialize simple struct
    {
        std::cout << "Test 8: Serialize simple struct... ";
        Point p{30, 40};
        std::string output;
        RapidYamlWriter writer(output);
        auto result = SerializeWithWriter(p, writer);
        assert(result.error() == SerializeError::NO_ERROR);
        writer.finish();
        std::cout << "PASSED\nOutput:\n" << output << "\n";
    }

    // Test 9: Serialize array
    {
        std::cout << "Test 9: Serialize array... ";
        std::vector<int> vec = {1, 2, 3};
        std::string output;
        RapidYamlWriter writer(output);
        auto result = SerializeWithWriter(vec, writer);
        assert(result.error() == SerializeError::NO_ERROR);
        writer.finish();
        std::cout << "PASSED\nOutput:\n" << output << "\n";
    }

    // Test 10: Serialize nested
    {
        std::cout << "Test 10: Serialize nested struct... ";
        Config cfg{"my config", 99, false, {5, 6, 7}};
        std::string output;
        RapidYamlWriter writer(output);
        auto result = SerializeWithWriter(cfg, writer);
        assert(result.error() == SerializeError::NO_ERROR);
        writer.finish();
        std::cout << "PASSED\nOutput:\n" << output << "\n";
    }

    std::cout << "\n=== All tests passed! ===\n";
    return 0;
}
