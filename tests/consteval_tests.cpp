#include <string_view>
#include <array>
#include <optional>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/parser.hpp>



int main() {
    struct A {
        int a = 10;
        bool b;
        std::array<int, 2> c;
        std::optional<float> empty_opt;
        std::optional<int> filled_opt;

        struct Nested {
            int nested_f;
            std::array<char, 10> nested_string;
        } nested;
    };
    static_assert([]() constexpr {
        A a;
        return JsonFusion::Parse(a, std::string_view(R"JSON(
                {
                    "a": 10,
                    "empty_opt": null,
                    "b": true,
                    "c": [5, 6],
                    "nested": {"nested_f": 18, "nested_string": "st"},
                    "filled_opt": 14

        }
        )JSON"))
            && a.a == 10
            && a.b
            && a.c[0] == 5 && a.c[1] == 6
            && !a.empty_opt
            && *a.filled_opt == 14
            && a.nested.nested_f == 18
            && a.nested.nested_string[0]=='s'&& a.nested.nested_string[1]=='t'
            ;
    }());
    static_assert([]() constexpr {
        A a{};
        a.b = true;
        a.filled_opt=18;
        a.nested.nested_f=-9;
        a.c[1]=118;
        a.nested.nested_string[0]='f';
        a.nested.nested_string[1]='u';
        std::array<char, 1000> buf;

        char * out_pos =buf.data();
        char * en = out_pos + buf.size();
        bool r = JsonFusion::Serialize(a, out_pos, en);

        std::string_view res(R"JSON({"a":10,"b":true,"c":[0,118],"empty_opt":null,"filled_opt":18,"nested":{"nested_f":-9,"nested_string":"fu"}})JSON");
        return r &&std::ranges::equal(buf.data(), out_pos, res.data(), res.end());
    }());
}
