#include <string_view>
#include <array>
#include <optional>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/parser.hpp>
#include <list>
#include <cassert>

#include <iostream>
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
    // static_assert([]() constexpr {
    //     A a;
    //     return JsonFusion::Parse(a, std::string_view(R"JSON(
    //             {
    //                 "a": 10,
    //                 "empty_opt": null,
    //                 "b": true,
    //                 "c": [5, 6],
    //                 "nested": {"nested_f": 18, "nested_string": "st"},
    //                 "filled_opt": 14

    //     }
    //     )JSON"))
    //         && a.a == 10
    //         && a.b
    //         && a.c[0] == 5 && a.c[1] == 6
    //         && !a.empty_opt
    //         && *a.filled_opt == 14
    //         && a.nested.nested_f == 18
    //         && a.nested.nested_string[0]=='s'&& a.nested.nested_string[1]=='t'
    //         ;
    // }());
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
    {

        static_assert(JsonFusion::static_schema::ArrayReadable<std::array<int, 5>>);
        static_assert(JsonFusion::static_schema::ArrayReadable<std::vector<int>>);
        static_assert(JsonFusion::static_schema::ArrayReadable<std::list<int>>);


        static_assert(JsonFusion::static_schema::JsonSerializableValue<std::array<int, 5>>);
        static_assert(JsonFusion::static_schema::JsonSerializableValue<std::vector<int>>);
        static_assert(JsonFusion::static_schema::JsonSerializableValue<std::list<int>>);



        struct Streamer {
            struct Vector{
                float x, y, z;
            };
            using value_type = JsonFusion::Annotated<Vector, JsonFusion::options::as_array>;
            int count;
            mutable int counter = 0;
            constexpr JsonFusion::stream_read_result read(Vector & v) const {
                if (counter >= count) {
                    return JsonFusion::stream_read_result::end;
                }
                counter ++;
                v.x = 42 + counter;
                v.y = 43 + counter;
                v.z = 44 + counter;
                return JsonFusion::stream_read_result::value;
            }
        };
        struct A {
            int f;
            JsonFusion::Annotated<Streamer, JsonFusion::options::key<"points_xyz">> int42items;
            std::vector<int> v = {1,23};
        };
        A a;
        a.f = -2;
        a.int42items->count=3;
        std::string out;
        assert(JsonFusion::Serialize(a, out));
        std::cout << out << std::endl;
    }
}
