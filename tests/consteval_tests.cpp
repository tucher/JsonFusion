#include <string_view>
#include <array>
#include <optional>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <list>
#include <cassert>
#include <format>
#include <iostream>

auto printErr = [](auto res, std::string_view js_sv) {
    int pos = res.pos() - js_sv.data();
    int wnd = 20;
    std::string before(js_sv.substr(pos+1 >= wnd ? pos+1-wnd:0, pos+1 >= wnd ? wnd:0));
    std::string after(js_sv.substr(pos+1, wnd));
    std::cerr << std::format("JsonFusion parse failed: error {} at {}: '...{}😖{}...'", int(res.error()), pos, before, after)<< std::endl;
};

int main() {
    struct Nested {
        int nested_f;
        std::array<char, 10> nested_string;
        std::array<char, 10> skip1;
        bool skip2;
    };

    struct A {
        int a = 10;
        bool b;
        std::array<int, 2> c;
        std::optional<float> empty_opt;
        std::optional<int> filled_opt;


        JsonFusion::Annotated<Nested, JsonFusion::validators::not_required<"skip1", "skip2">> nested;
        std::vector<int> dynamic_array;
        std::string dynamic_string;
        std::vector<std::vector<int>> vec_of_vec_of_int;
        std::vector<std::optional<std::vector<std::string>>> vec_of_opt_vecs;

    };
    static_assert([]() constexpr {
        A a;
        return JsonFusion::Parse(a, std::string(R"JSON(
                {
                    "a": 10,
                    "empty_opt": null,
                    "b": true,
                    "c": [5, 6],
                    "nested": {"nested_f": 18, "nested_string": "st"},
                    "filled_opt": 14,
                    "dynamic_string": "variable string",
                    "dynamic_array": [1],
                    "vec_of_vec_of_int":[[2]],
                    "vec_of_opt_vecs": [null, ["a","b","c"], null]
        }
        )JSON"))
            && a.a == 10
            && a.b
            && a.c[0] == 5 && a.c[1] == 6
            && !a.empty_opt
            && *a.filled_opt == 14
            && a.nested->nested_f == 18
            && a.nested->nested_string[0]=='s'&& a.nested->nested_string[1]=='t'
            && a.dynamic_string == "variable string"
            && a.dynamic_array[0] == 1
            && a.vec_of_vec_of_int[0][0]==2
            && !a.vec_of_opt_vecs[0] && !a.vec_of_opt_vecs[2]
               && a.vec_of_opt_vecs[1] && *a.vec_of_opt_vecs[1] == std::vector<std::string>{"a", "b", "c"}
            ;
    }());
    static_assert([]() constexpr {
        A a{};
        a.b = true;
        a.filled_opt=18;
        a.nested->nested_f=-9;
        a.c[1]=118;
        a.nested->nested_string[0]='f';
        a.nested->nested_string[1]='u';
        a.dynamic_array = {12,34};
        a.dynamic_string = "str";
        a.vec_of_vec_of_int = {{3}};
        a.vec_of_opt_vecs = {std::nullopt, std::vector<std::string>{"a","b","c"} , std::nullopt};
        std::string out;
        bool r = JsonFusion::Serialize(a, out);

        return r &&
               out == R"JSON({"a":10,"b":true,"c":[0,118],"empty_opt":null,"filled_opt":18,"nested":{"nested_f":-9,"nested_string":"fu","skip1":"","skip2":false},"dynamic_array":[12,34],"dynamic_string":"str","vec_of_vec_of_int":[[3]],"vec_of_opt_vecs":[null,["a","b","c"],null]})JSON";
    }());
    static_assert([]() constexpr {
        struct Consumer {
            struct Tag {
                std::string id;
                std::string text;
            };
            using value_type = Tag;

            // Called at the start of the JSON array
            constexpr void reset()  {

            }

            // Called for each element, with a fully-parsed value_type
            constexpr bool consume(const Tag & tag)  {

                return true;
            }

            // called once at the end (with json success flag, if true, all data was consumed successfully)
            constexpr bool finalize(bool success)  {
                if (!success) {

                    return false;
                }
                return true;
            }

        };
        Consumer t{};
        return JsonFusion::Parse(t, std::string_view(R"JSON([
            {"id": "1", "text": "first tag"},
            {"id": "2", "text": "second tag"}
        ]
        )JSON"));

    }());
    static_assert([]() constexpr {
        struct Producer {
            using value_type = int64_t;

            int count = 5;
            mutable int counter = 0;

            constexpr JsonFusion::stream_read_result read(double & v) const {
                if (counter >= count) {
                    return JsonFusion::stream_read_result::end;
                }
                counter ++;
                v = counter;
                return JsonFusion::stream_read_result::value;
            }

            void reset() const {
                counter = 0;
            }

        };
        Producer t{};
        std::array<char, 1000> buf;

        char * out_pos =buf.data();
        char * en = out_pos + buf.size();
        bool r = JsonFusion::Serialize(t, out_pos, en);
        return r;
    }());



}
