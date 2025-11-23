
#include <cassert>
#include <list>
#include <iostream>

#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>

void schema_tests() {
    using std::string, std::list, std::vector, std::array, std::optional;
    using namespace JsonFusion;
    using namespace options;
    struct Module2 {
        // Annotated<string, "path"> path;
        Annotated<int, key<"name">,
                  not_required,
                  range<0, 100>,
                  description<"Velocity in m/s">
                  > count;
        Annotated<optional<bool>, key<"is_absolute">> not_relative;
    };

    struct Root2 {
        Annotated<list<Module2>, key<"members">> members;
    };
    Root2 test;


    static_assert(static_schema::JsonValue<Root2>);
    {
        using namespace JsonFusion::static_schema;
        static_assert(JsonValue<bool>);
        static_assert(!JsonNullableValue<bool>);
        static_assert(is_non_null_json_value<bool>::value);
        static_assert(JsonValue<int>);
        static_assert(JsonValue<char>);
        static_assert(JsonValue<float>);
        static_assert(JsonValue<double>);
        static_assert(JsonValue<string>);

        static_assert( JsonString<string>);
        static_assert(!JsonArray<string>);

        static_assert(JsonNullableValue<optional<bool>>);
        static_assert(JsonNullableValue<optional<int>>);
        static_assert(JsonNullableValue<optional<char>>);
        static_assert(JsonNullableValue<optional<float>>);
        static_assert(JsonNullableValue<optional<double>>);
        static_assert(JsonNullableValue<optional<string>>);


        static_assert(JsonValue<list<bool>>);
        static_assert(JsonArray<vector<string>>);
        static_assert(JsonNullableValue<optional<list<bool>>>);

        static_assert(JsonArray<vector<optional<int>>>);

        static_assert(JsonValue<Annotated<bool>>);

        // using A = Annotated<int>;
        static_assert(static_schema::JsonNullableValue<std::optional<int>>);
        static_assert(static_schema::JsonNullableValue<Annotated<std::optional<int>>>);

        // This should *not* be a valid JSON value anymore:
        using Bad = std::optional<Annotated<int>>;
        // static_assert(!static_schema::JsonValue<Bad>);

        static_assert(JsonNullableValue<std::optional<int>>);
        static_assert(JsonNullableValue<Annotated<std::optional<int>>>);
        // static_assert(!JsonValue<std::optional<Annotated<int>>>);

        struct SimpleObject {
            bool b;
        };
        static_assert(JsonValue<SimpleObject>);

        struct EmptyRecursiveObject {
            list<EmptyRecursiveObject> children;
        };
        static_assert(JsonValue<EmptyRecursiveObject>);

        struct RecursiveObject {
            int data;
            list<RecursiveObject> children;
        };
        static_assert(JsonValue<RecursiveObject>);



        struct Node {
            struct NodeOpts {
                list<Node> children;
                bool optV;
            };
            string data;
            NodeOpts opts;
        };

        static_assert(JsonValue<Node>);

        struct B;
        struct A {
            Annotated<bool, key<"is_absolute">> not_relative;
            list<Annotated<B>> tst;
        };


        static_assert(JsonValue<A>);
        static_assert(JsonValue<optional<A>>);

        struct B {
            bool field;
            optional<int> optional_field;
            list<A> list1;
            optional<list<A>> list2;
            optional<list<optional<A>>> list3;
            vector<B> arr;
        };

        static_assert(JsonValue<B>);
        static_assert(JsonValue<optional<B>>);


        static_assert(JsonValue<list<list<bool>>>);
        static_assert(JsonValue<list<list<B>>>);

        static_assert(JsonValue<
                      optional<
                          list<
                              optional<
                                  list<optional<B>>
                                  >
                              >
                          >
                      >);

        struct EmptyNode {
            std::vector<EmptyNode> children;
        };

        static_assert(JsonValue<EmptyNode>);

        struct C { Annotated<int> x; };
        static_assert( JsonValue<C>);
        static_assert( JsonValue<optional<C>>);
        static_assert( JsonNullableValue<optional<C>>);



    }
    {
        using namespace JsonFusion;
        using namespace JsonFusion::options;


        using opts = options::detail::annotation_meta_getter<Annotated<int, not_required, key<"fuu">, range<2,3>>>::options;
        static_assert(opts::has_option<options::detail::key_tag>
                      && opts::get_option<options::detail::key_tag>::desc.toStringView() == "fuu");

        static_assert(opts::has_option<options::detail::range_tag>
                      && opts::get_option<options::detail::range_tag>::min == 2
                      && opts::get_option<options::detail::range_tag>::max == 3);

        static_assert(opts::has_option<options::detail::not_required_tag>);
        static_assert(!opts::has_option<options::detail::description_tag>);
    }
}
void test() {
    schema_tests();
    using std::string, std::list, std::vector, std::array, std::optional;
    using namespace JsonFusion;
    using namespace JsonFusion::options;
    struct Module2 {
        // JsonFusion::Annotated<string, "path"> path;
        Annotated<int, key<"name">,
                  not_required,
                  range<0, 100>,
                  description<"Velocity in m/s">
                  > count;
        Annotated<optional<bool>, key<"is_absolute">> not_relative;
    };

    struct Root2 {
        Annotated<list<Module2>, key<"members">> members;
    };
    Root2 test;
    {
        bool bool_v;
        assert(Parse(bool_v, std::string_view("true")) && bool_v);
        assert(Parse(bool_v, std::string_view("false")) && !bool_v);
    }
    {
        std::optional<bool> bool_opt_v;
        assert(Parse(bool_opt_v, std::string_view("true")) && bool_opt_v && *bool_opt_v);
        assert(Parse(bool_opt_v, std::string_view("false")) && bool_opt_v && !*bool_opt_v);
        assert(Parse(bool_opt_v, std::string_view("null")) && !bool_opt_v);
    }
    {
        int iv;
        std::optional<int> opt_iv;

        assert(Parse(iv, std::string_view("100")) && iv == 100);
        assert(Parse(opt_iv, std::string_view("100")) && opt_iv && *opt_iv == 100);
        assert(Parse(opt_iv, std::string_view("null")) && !opt_iv);

        float fv;
        std::optional<float> opt_fv;
        auto almost_equal = [] (float a, float b, float epsilon = 0.0001f) {
            return std::fabs(a - b) < epsilon;
        };
        assert(Parse(fv, std::string_view("100.1")) && almost_equal(fv, 100.1));
        assert(Parse(opt_fv, std::string_view("100.1")) && opt_fv && almost_equal(*opt_fv, 100.1));
        assert(Parse(opt_fv, std::string_view("null")) && !opt_fv);
    }
    {
        std::string ds;
        assert(Parse(ds, std::string_view("\"100\"")) && ds == "100");
        std::array<char, 20> fs;
        assert(Parse(fs, std::string_view("\"100\"")) && std::string(fs.data()) == "100");

        Annotated<string, min_length<5>, max_length<10>> as;
        assert(!Parse(as, std::string_view("\"100\"")));
        assert(!Parse(as, std::string_view("\"123456789012345\"")));
        assert(Parse(as, std::string_view("\"hellowrld\"")));


    }
    {
        std::string unicode;
        assert(Parse(unicode, std::string_view(R"( "simple\ntext\twith\\escape\"" )")));
        assert(unicode == "simple\ntext\twith\\escape\"");

        assert(Parse(unicode, std::string_view(R"( "Caf\u00E9" )")));
        assert(unicode == "Café");

        assert(Parse(unicode, std::string_view(R"( "\u041F\u0440\u0438\u0432\u0435\u0442" )")));
        assert(unicode == "Привет");

        assert(Parse(unicode, std::string_view(R"( "\uD83D\uDE00" )")));
        assert(unicode == "😀");
    }
    {
        Annotated<std::optional<int8_t>,  range<0, 100>> minValue;

        assert(Parse(minValue, std::string_view("99")));
        assert(!Parse(minValue, std::string_view("128")));
        assert(!Parse(minValue, std::string_view("-1")));
    }
    {
        std::vector<int> ds;
        std::vector<int> expected = {1, 2, 3};
        assert(Parse(ds, std::string_view("[1, 2, 3]")) && ds == expected);

        std::array<int, 3> fs;
        std::array<int, 3> expectedfs = {1, 2, 3};
        assert(Parse(fs, std::string_view("[1, 2, 3]")) && fs == expectedfs);

        std::array<int, 5> fs2{};
        std::array<int, 5> expectedfs2 = {1, 2, 3};
        assert(Parse(fs2, std::string_view("[1, 2, 3]")) && fs2 == expectedfs2);

        Annotated<list<int>, min_items<3>, max_items<6>> arr_with_limits;
        assert(!Parse(arr_with_limits, std::string_view("[1, 2]")));
        assert(!Parse(arr_with_limits, std::string_view("[1, 2, 3, 4, 5, 6, 7]")));
        assert(Parse(arr_with_limits, std::string_view("[1, 2, 3, 4]")));

    }

    {
        struct Point {
            Annotated<bool, not_json> skip_me;
            float x;
            Annotated<bool, not_json> skip_me2;
            float y, z;
            Annotated<bool, not_json> skip_me_too;
        };
        list<Annotated<Point, as_array>> ob;
        assert(Parse(ob,  std::string_view(R"(
[
[1, 2, 3],
[5, 6, 7],
[8, 9, 10]
]
)")));

        string output;


        assert(Serialize(ob, output) );
        int a = 1;
    }
    struct ob {
        int b;
        Annotated<int, key<"new_key">, range<2, 100>, not_required> c;
        Annotated<list<bool>, min_items<3> > flags;

        struct Inline {
            int inline_field;
        } inlined;
    };
    ob structure;

    Parse(structure,  std::string_view(R"(
        {
            "b": 123,
            "new_key": 10,
            "flags": [false, true, false, true],
            "inlined": {"inline_field": 42}
        }
    )"));

    struct sink{};
    Annotated<sink, allow_excess_fields> test_sink;
    Parse(test_sink,  std::string_view(R"(
        {
            "b": 123,
            "new_key": 10,
            "flags": [false, true, false, true],
            "inlined": {"inline_field": 42}
        }
    )"));



    {
        struct A {
            Annotated<int, options::key<"f">> field;
            Annotated<string> opt;
            vector<std::optional<std::int64_t>> vect;
            Annotated<A**, not_json> fuuu2;
            Annotated<bool, not_required> may_be_missing;
            Annotated<A*, not_json> fuuu;
        };
        A a;
        assert(Parse(a, std::string_view(R"(
            {
                "opt": "213",
                "f": 123,
                "vect": [12, -100, null  ]
            }
        )")));
        Annotated<string> tst;
        string s;
        assert(s == "");
        assert(tst == "");
        assert(a.opt[1]=='1');
        assert(a.field == 123
               && a.opt.value == "213" && a.vect.size()==3 && *a.vect[0]==12 && *a.vect[1]==-100 && !a.vect[2]
               );


        {
            using std::array;
            using std::int64_t;
            using std::list;
            using std::optional;
            using std::string;
            using std::vector;

            // Simple scalar/object combo used in a few places
            struct Limits {
                int    minValue;
                int    maxValue;
            };

            // Fixed-size “string-like” fields exercise JsonString on non-dynamic containers
            struct FixedStrings {
                array<char, 8>  code;   // e.g. "CFG001"
                array<char, 16> label;  // e.g. "MainConfig"
            };

            // Recursive node with arrays, optionals, nested nodes
            struct Node {
                string                    name;       // dynamic string
                bool                      active;     // bool
                vector<int>               weights;    // dynamic array of numbers
                optional<double>          bias;       // optional number
                vector<optional<bool>>    flags;      // array of optional bools
                vector<Node>              children;   // recursive objects
            };

            // Credentials-type object to test small nested object
            struct Credentials {
                string             user;
                optional<string>   password;   // optional string
            };

            // The main “kitchen sink” config
            struct ComplexConfig {
                // Scalars
                bool         enabled;
                char         mode;             // parsed as JsonNumber (integral)
                int          retryCount;
                double       timeoutSeconds;

                // Strings
                string       title;            // dynamic string

                FixedStrings fixedStrings;     // object containing fixed-size char arrays

                // Arrays
                array<int, 3>           rgb;          // fixed-size numeric array
                vector<string>          tags;         // dynamic array of strings
                list<int64_t>           counters;     // dynamic list of numbers
                vector<vector<int>>     matrix;       // nested arrays

                // Optionals (scalars / arrays / objects)
                optional<int>           debugLevel;   // optional number
                optional<string>        optionalNote; // optional string
                optional<vector<int>>   optionalArray;// optional array of numbers
                optional<Limits>        optionalLimits;

                // Nested objects
                Limits                  hardLimits;
                Credentials             creds;

                // Recursive object graph and containers of objects
                Node                    rootNode;
                vector<Node>            extraNodes;
                optional<Node>          optionalNode;
                vector<optional<Node>>  nodeHistory;  // array of optional objects
            };
            ComplexConfig test;
            assert(Parse(test, std::string_view(R"(
{
  "enabled": true,
  "mode": 1,
  "retryCount": 3,
  "timeoutSeconds": 1.5,

  "title": "Main \\\"config\\\" example",

  "fixedStrings": {
    "code": "CFG001",
    "label": "MainConfig"
  },

  "rgb": [255, 128, 64],

  "tags": [
    "alpha",
    "beta",
    "gamma"
  ],

  "counters": [
    1,
    2,
    9999999999
  ],

  "matrix": [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
  ],

  "debugLevel": 42,
  "optionalNote": null,
  "optionalArray": [10, 20, 30],
  "optionalLimits": {
    "minValue": 5,
    "maxValue": 95
  },

  "hardLimits": {
    "minValue": 0,
    "maxValue": 100
  },

  "creds": {
    "user": "admin",
    "password": "secret"
  },

  "rootNode": {
    "name": "root",
    "active": true,
    "weights": [1, 2, 3, 4],
    "bias": 0.5,
    "flags": [true, null, false],
    "children": [
      {
        "name": "child1",
        "active": false,
        "weights": [],
        "bias": null,
        "flags": [],
        "children": []
      },
      {
        "name": "child2",
        "active": true,
        "weights": [5, 6],
        "bias": -1.25,
        "flags": [null, true],
        "children": []
      }
    ]
  },

  "extraNodes": [
    {
      "name": "extra1",
      "active": true,
      "weights": [10],
      "bias": null,
      "flags": [false],
      "children": []
    },
    {
      "name": "extra2",
      "active": false,
      "weights": [],
      "bias": 3.1415,
      "flags": [],
      "children": []
    }
  ],

  "optionalNode": {
    "name": "optional",
    "active": true,
    "weights": [7, 8, 9],
    "bias": 2.718,
    "flags": [false, true],
    "children": []
  },

  "nodeHistory": [
    null,
    {
      "name": "history1",
      "active": false,
      "weights": [100],
      "bias": null,
      "flags": [],
      "children": []
    },
    null
  ]
}

            )")));

            ComplexConfig t2 = test;
        }

    }

}

struct TrivialSentinel {};

template<typename Iterator>
bool operator==(Iterator it, TrivialSentinel) {
    return it == Iterator{};  // or implement your end condition check here
}

template<typename Iterator>
bool operator!=(Iterator it, TrivialSentinel s) {
    return !(it == s);
}

void serialize_tests() {
    using std::string, std::list, std::vector, std::array, std::optional;
    using namespace JsonFusion;
    using namespace JsonFusion::options;

    string output;


    assert(Serialize(true, output) && string(output.c_str()) == "true");

    assert(Serialize(false, output) && string(output.c_str()) == "false");

    assert(Serialize(optional<bool>{}, output) && string(output.c_str()) == "null");

    assert(Serialize(optional<bool>{true}, output) && string(output.c_str()) == "true");

    assert(Serialize(list<bool>{true, false, true}, output) && string(output.c_str()) == "[true,false,true]");


    assert(Serialize(int(12345), output) && string(output.c_str()) == "12345");


    assert(Serialize(float(3.14), output) && string(output.c_str()) == "3.140000104904175");

    struct A {
        int a = 12;
        optional<string> b = {};
        list<bool> flags = {false, true, false};
    };

    assert(Serialize(A(), output) &&
           string(output.c_str()) == R"({"a":12,"b":null,"flags":[false,true,false]})");

    struct B {
        Annotated<int, key<"field1">> a{12};
        optional<string> b = {};
        list<bool> flags = {false, true, false};
    };

    assert(Serialize(B(), output) &&
           string(output.c_str()) == R"({"field1":12,"b":null,"flags":[false,true,false]})");

}
int main()
{
    test();
    serialize_tests();
}
