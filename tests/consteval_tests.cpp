#define JSONFUSION_ALLOW_JSON_PATH_STRING_ALLOCATION_FOR_MAP_ACCESS
#define JSONFUSION_ALLOW_DYNAMIC_ERROR_STACK

#include <string_view>
#include <array>
#include <optional>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <cassert>
#include <format>
#include <iostream>
#include <map>
#include "JsonFusion/json_path.hpp"
#include "JsonFusion/error_formatting.hpp"
#include "JsonFusion/string_search.hpp"

using JsonFusion::A;

using namespace JsonFusion::validators;
using namespace JsonFusion::options;

int main() {

    JsonFusion::string_search::test();

    struct Nested {
        int nested_f;
        std::array<char, 10> nested_string;
        std::array<char, 10> skip1;
        bool skip2;
    };

    struct Model {
        int a = 10;
        bool b;
        std::array<int, 2> c;
        std::optional<float> empty_opt;
        std::optional<int> filled_opt;


        A<Nested, not_required<"skip1", "skip2">> nested;
        std::vector<int> dynamic_array;
        std::string dynamic_string;
        std::vector<std::vector<int>> vec_of_vec_of_int;
        std::vector<std::optional<std::vector<std::string>>> vec_of_opt_vecs;

    };
    static_assert([]() constexpr {
        Model a;
        return JsonFusion::Parse(a, std::string_view(R"JSON(
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
        Model a{};
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
    static_assert([]() constexpr {
        struct Tst21{
            A<bool, constant<true>> bool_const_t;
            A<bool, constant<false>> bool_const_f;
            A<std::array<char, 5>, string_constant<"fu">> string_c;
            A<int, constant<42>> number_const;
        } a;
        return JsonFusion::Parse(a, std::string_view(R"JSON(
                    {
                        "bool_const_t": true,
                        "bool_const_f": false,
                        "string_c": "fu",
                        "number_const": 42
            }
            )JSON"))
            ;
    }());

    static_assert([]() constexpr {
        struct Root {
            int field;

            struct Inner1 {
                int field;

                struct Inner2 {
                    int field;
                };
                std::array<Inner2, 3> inners;
            };

            std::vector<Inner1> inners;
        };
        /*
         Inner1 [0]
        Inner1.field [1]
        Inner1.inners [1]
        Inner1.inners[0] [2]
        Inner1.inners[1][0] [3]
         */
        static_assert(JsonFusion::schema_analyzis::calc_type_depth<Root::Inner1::Inner2>() == 2);
        static_assert(JsonFusion::schema_analyzis::calc_type_depth<Root::Inner1>() == 4);
        static_assert(JsonFusion::schema_analyzis::calc_type_depth<Root>() == 6);
        return 1;
    }());
    static_assert([]() constexpr {
        struct Root1 {
            int field;
            std::vector<Root1> inners;
            std::unique_ptr<Root1> child;
        };

        struct Root2 {
            int field;
            std::vector<Root1> inners;
            std::unique_ptr<Root1> child;
        };
        static_assert(JsonFusion::schema_analyzis::calc_type_depth<Root1>() == JsonFusion::schema_analyzis::SCHEMA_UNBOUNDED);
        static_assert(JsonFusion::schema_analyzis::calc_type_depth<Root2>() == JsonFusion::schema_analyzis::SCHEMA_UNBOUNDED);
        return 1;
    }());

    {
        A<std::map<std::string, int>,
                              required_keys<"1", "text">,
                              min_properties<2>

                              > t;
        auto res =  JsonFusion::Parse(t, std::string_view(R"JSON(
                {"1": 1, "text": 2, "fuu": 3}


            )JSON"));
        assert(res);
    }
    {
        A<std::map<std::string, int>,
                              required_keys<"1", "text">,
                              min_properties<2>

                              > t;
        auto res =  JsonFusion::Parse(t, std::string_view(R"JSON(
                {"1": 1,  "fuu": 3}


            )JSON"));
        assert(!res);
    }
    {
        A<std::map<std::string, int>,
                              allowed_keys<"1", "text">

                              > t;
        assert(!JsonFusion::Parse(t, std::string_view(R"JSON(
                {"1": 1, "text": 2, "fuu": 3}


            )JSON")));
    }
    {
        A<std::map<std::string, int>,
                              allowed_keys<"1", "text", "fuu">

                              > t;
        assert(JsonFusion::Parse(t, std::string_view(R"JSON(
                {"1": 1, "text": 2}


            )JSON")));
    }
    {
        A<std::map<std::string, int>,
                              forbidden_keys<"1", "text", "fuu">

                              > t;
        assert(!JsonFusion::Parse(t, std::string_view(R"JSON(
                {"1": 1, "text": 2}


            )JSON")));
    }
    {
        A<std::map<std::string, int>,
                              forbidden_keys<"1", "text", "fuu">

                              > t;
        assert(JsonFusion::Parse(t, std::string_view(R"JSON(
                {"11": 1, "text1": 2}


            )JSON")));
    }
    {
        A<bool,
                              constant<true>

                              > t;
        assert(JsonFusion::Parse(t, std::string_view(R"JSON(
                true


            )JSON")));
    }
    {
        A<bool,
                              constant<false>

                              > t;
        assert(JsonFusion::Parse(t, std::string_view(R"JSON(
                false


            )JSON")));
    }
    {
        struct TS{
            A<bool, constant<true>> bool_const_t;
            A<bool, constant<false>> bool_const_f;
            A<std::array<char, 10>, string_constant<"I am str">> string_c;
            A<int, constant<42>> number_const;
            struct Inner{
                double f;
            };
            std::vector<Inner> inner;
        } a;
        std::string_view sv{R"JSON(
                {
                    "bool_const_t": true,
                    "bool_const_f": false,
                    "string_c": "I am str",
                    "number_const": 42,
                    "inner": [{"f": 4.3},{"f": true}]
        }
        )JSON"
        };
        auto r = JsonFusion::Parse(a, sv);
        if(!r) {
            std::cerr << ParseResultToString<TS>(r, sv.begin(), sv.end()) << std::endl;
            auto jp = JsonFusion::json_path::JsonPath<4, false>("inner", 0, "f");
            assert(jp.currentLength == 3);


            JsonFusion::json_path::visit_by_path(a, []<class T>(T &v, auto Opts) {
                if constexpr(std::is_same_v<T, double>) {
                    v = 123.456;
                }
            }, jp);

            if(a.inner.size() > 0)
                std::cout << a.inner[0].f << std::endl;


            JsonFusion::json_path::visit_by_path(a, []<class T>(T &v, auto Opts) {
                if constexpr(std::is_same_v<T, double>) {
                    v = 1.4;
                }
            }, jp);

            if(a.inner.size() > 0)
                std::cout << a.inner[0].f << std::endl;
        }

    }

    {

        struct TS{
            struct Inner{
                double f;
                std::map<std::string, bool> bools;
            };
            std::vector<Inner> inner;
        } a;

        static_assert (JsonFusion::schema_analyzis::has_maps<TS>());
        std::string_view sv{R"JSON(
        {

            "inner": [
                {
                    "f": 4.3,
                    "bools": {"укп": false, "укпук": false, "укпукп": 34}
                },
                {
                    "bools": {"счмчсм": false, "чсм": false, "кеи": true}
                }
            ]
        }
        )JSON"
        };
        auto r = JsonFusion::Parse(a, sv);
        if(!r) {
            std::cerr << ParseResultToString<TS>(r, sv.begin(), sv.end()) << std::endl;
           
        }

    }
    {

        struct TS{
            struct Inner{
                double f;
                std::map<std::string, TS> children;
            };
            std::vector<Inner> inner;
        } a;

        static_assert (JsonFusion::schema_analyzis::has_maps<TS>());
        std::string_view sv{R"JSON(
        {

            "inner": [
                {
                    "f": 4.3,
                    "children": {"first1": {"inner": []}, "first2": {"inner": []}}
                },
                {
                    "f": 4.8,
                    "children": {"second1": {"inner": [{"f": 5.4, "children": null}]}, "second2": {"inner": []}}
                }
            ]
        }
        )JSON"
        };
        auto r = JsonFusion::Parse(a, sv);
        if(!r) {
            std::cerr << ParseResultToString<TS>(r, sv.begin(), sv.end()) << std::endl;
          
        }

    }

    {

        struct TS{
            bool val_b;
            A<std::array<char, 512>, JsonFusion::options::json_sink<64, 5>> sink;
            A<int, constant<42>> number_const;
            struct Inner{
                double f;
            };
            std::vector<Inner> inner;
        } a;
        std::string_view sv{R"JSON(
                {
                    "val_b": true,
                    "sink": [[[[1, 2, 3]]]],
                    "number_const": 42,
                    "inner": [{"f": 4.3},{"f": 2.3}]
        }
        )JSON"
        };
        auto r = JsonFusion::Parse(a, sv);
        if(!r) {
            std::cerr << ParseResultToString<TS>(r, sv.begin(), sv.end()) << std::endl;

        } else {
            std::cout << "In sink: " << std::string(a.sink.data()) << std::endl;
            // OUTPUT: In sink: [[[[1,2,3]]]]
        }

    }



}
