#include "static_schema.hpp"
#include "parser.hpp"
#include <cassert>
#include <list>
#include <iostream>

void schema_tests() {
    using std::string, std::list, std::vector, std::array, std::optional;
    using namespace JSONReflection2;
    using namespace JSONReflection2::options;
    struct Module2 {
        // JSONReflection2::Annotated<string, "path"> path;
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


    static_assert(JSONReflection2::static_schema::JsonValue<Root2>);
    {
        using namespace JSONReflection2::static_schema;
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
        using namespace JSONReflection2;
        using namespace JSONReflection2::options;


        using opts = options::detail::field_meta_decayed<Annotated<int, not_required, key<"fuu">, range<2,3>>>::options;
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
    using namespace JSONReflection2;
    using namespace JSONReflection2::options;
    struct Module2 {
        // JSONReflection2::Annotated<string, "path"> path;
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
        assert(JSONReflection2::Parse(bool_v, std::string_view("true")) && bool_v);
        assert(JSONReflection2::Parse(bool_v, std::string_view("false")) && !bool_v);
    }
    {
        std::optional<bool> bool_opt_v;
        assert(JSONReflection2::Parse(bool_opt_v, std::string_view("true")) && bool_opt_v && *bool_opt_v);
        assert(JSONReflection2::Parse(bool_opt_v, std::string_view("false")) && bool_opt_v && !*bool_opt_v);
        assert(JSONReflection2::Parse(bool_opt_v, std::string_view("null")) && !bool_opt_v);
    }
    {
        int iv;
        std::optional<int> opt_iv;

        assert(JSONReflection2::Parse(iv, std::string_view("100")) && iv == 100);
        assert(JSONReflection2::Parse(opt_iv, std::string_view("100")) && opt_iv && *opt_iv == 100);
        assert(JSONReflection2::Parse(opt_iv, std::string_view("null")) && !opt_iv);

        float fv;
        std::optional<float> opt_fv;
        auto almost_equal = [] (float a, float b, float epsilon = 0.0001f) {
            return std::fabs(a - b) < epsilon;
        };
        assert(JSONReflection2::Parse(fv, std::string_view("100.1")) && almost_equal(fv, 100.1));
        assert(JSONReflection2::Parse(opt_fv, std::string_view("100.1")) && opt_fv && almost_equal(*opt_fv, 100.1));
        assert(JSONReflection2::Parse(opt_fv, std::string_view("null")) && !opt_fv);
    }
    {
        std::string ds;
        assert(JSONReflection2::Parse(ds, std::string_view("\"100\"")) && ds == "100");
        std::array<char, 20> fs;
        assert(JSONReflection2::Parse(fs, std::string_view("\"100\"")) && std::string(fs.data()) == "100");

        Annotated<string, min_length<5>, max_length<10>> as;
        assert(!JSONReflection2::Parse(as, std::string_view("\"100\"")));
        assert(!JSONReflection2::Parse(as, std::string_view("\"123456789012345\"")));
        assert(JSONReflection2::Parse(as, std::string_view("\"hellowrld\"")));
    }

    {
        using JSONReflection2::options::range, JSONReflection2::Annotated;
        Annotated<std::optional<int8_t>,  range<0, 100>> minValue;

        assert(JSONReflection2::Parse(minValue, std::string_view("99")));
        assert(!JSONReflection2::Parse(minValue, std::string_view("128")));
        assert(!JSONReflection2::Parse(minValue, std::string_view("-1")));
    }
    {
        std::vector<int> ds;
        std::vector<int> expected = {1, 2, 3};
        assert(JSONReflection2::Parse(ds, std::string_view("[1, 2, 3]")) && ds == expected);

        std::array<int, 3> fs;
        std::array<int, 3> expectedfs = {1, 2, 3};
        assert(JSONReflection2::Parse(fs, std::string_view("[1, 2, 3]")) && fs == expectedfs);

        std::array<int, 5> fs2{};
        std::array<int, 5> expectedfs2 = {1, 2, 3};
        assert(JSONReflection2::Parse(fs2, std::string_view("[1, 2, 3]")) && fs2 == expectedfs2);

        Annotated<list<int>, min_items<3>, max_items<6>> arr_with_limits;
        assert(!JSONReflection2::Parse(arr_with_limits, std::string_view("[1, 2]")));
        assert(!JSONReflection2::Parse(arr_with_limits, std::string_view("[1, 2, 3, 4, 5, 6, 7]")));
        assert(JSONReflection2::Parse(arr_with_limits, std::string_view("[1, 2, 3, 4]")));

    }
    {
        struct A {
            Annotated<int, options::key<"f">> field;
            Annotated<string> opt;
            vector<std::optional<std::int64_t>> vect;
            Annotated<bool, not_required> may_be_missing;
        };
        A a;
        assert(JSONReflection2::Parse(a, std::string_view(R"(
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
            assert(JSONReflection2::Parse(test, std::string_view(R"(
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

int main()
{
    test();
}
