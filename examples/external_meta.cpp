#include <JsonFusion/serializer.hpp>
#include <JsonFusion/parser.hpp>
using JsonFusion::Annotated;
using JsonFusion::options::as_array, JsonFusion::options::not_json;
#include <iostream>
#include <format>
using std::cout;
using std::endl;
using std::format;


struct Vec {
    float x = 1, y = 2, z = 3;
};
template<> struct JsonFusion::Annotated<Vec> {
    using Options = OptionsPack<
        as_array
        >;
};

template<> struct JsonFusion::AnnotatedField<Vec, 1> {
    using Options = OptionsPack<
        not_json
        >;
};


int main() {
    struct TopLevel {
        struct VecInner {
            float x = 4, y = 5, z = 6;
        };
        Annotated<VecInner, as_array> vec1;
        Vec vec2;
    };
    std::string out;
    JsonFusion::Serialize(TopLevel{}, out);
    cout << out << endl;
    /* {"vec1":[4,5,6],"vec2":[1,3]} */
    TopLevel t;
    JsonFusion::Parse(t, out);
    cout << format("vec1: ({}, {}, {}), vec2: ({}, {}, {})", t.vec1->x, t.vec1->y, t.vec1->z, t.vec2.x, t.vec2.y, t.vec2.z) << endl;
    /* vec1: (4, 5, 6), vec2: (1, 2, 3) */
}