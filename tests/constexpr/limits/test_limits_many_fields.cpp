#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <string>
#include <array>

using namespace TestHelpers;
using namespace JsonFusion;

// ============================================================================
// Test: Many Fields Limits
// ============================================================================

// Test 1: Struct with 50 fields
struct ManyFields50 {
    int f0, f1, f2, f3, f4, f5, f6, f7, f8, f9;
    int f10, f11, f12, f13, f14, f15, f16, f17, f18, f19;
    int f20, f21, f22, f23, f24, f25, f26, f27, f28, f29;
    int f30, f31, f32, f33, f34, f35, f36, f37, f38, f39;
    int f40, f41, f42, f43, f44, f45, f46, f47, f48, f49;
};

constexpr bool test_many_fields_50() {
    ManyFields50 obj{};
    // Manually constructed JSON for 50 fields
    constexpr std::string_view json {R"({"f0":0,"f1":10,"f2":20,"f3":30,"f4":40,"f5":50,"f6":60,"f7":70,"f8":80,"f9":90,"f10":100,"f11":110,"f12":120,"f13":130,"f14":140,"f15":150,"f16":160,"f17":170,"f18":180,"f19":190,"f20":200,"f21":210,"f22":220,"f23":230,"f24":240,"f25":250,"f26":260,"f27":270,"f28":280,"f29":290,"f30":300,"f31":310,"f32":320,"f33":330,"f34":340,"f35":350,"f36":360,"f37":370,"f38":380,"f39":390,"f40":400,"f41":410,"f42":420,"f43":430,"f44":440,"f45":450,"f46":460,"f47":470,"f48":480,"f49":490})"};
    
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    
    // Verify first, middle, and last fields
    return obj.f0 == 0
        && obj.f25 == 250
        && obj.f49 == 490;
}
static_assert(test_many_fields_50(), "Struct with 50 fields");

// Test 2: Struct with mixed types (30 fields)
struct MixedFields30 {
    int i0, i1, i2, i3, i4;
    bool b0, b1, b2, b3, b4;
    std::string s0, s1, s2, s3, s4;
    int i5, i6, i7, i8, i9;
    bool b5, b6, b7, b8, b9;
    std::string s5, s6, s7, s8, s9;
};

constexpr bool test_mixed_fields_30() {
    MixedFields30 obj{};
    constexpr std::string_view json {R"({"i0":1,"i1":2,"i2":3,"i3":4,"i4":5,"b0":true,"b1":false,"b2":true,"b3":false,"b4":true,"s0":"a","s1":"b","s2":"c","s3":"d","s4":"e","i5":6,"i6":7,"i7":8,"i8":9,"i9":10,"b5":false,"b6":true,"b7":false,"b8":true,"b9":false,"s5":"f","s6":"g","s7":"h","s8":"i","s9":"j"})"};
    
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    
    return obj.i0 == 1 && obj.i9 == 10
        && obj.b0 == true && obj.b9 == false
        && obj.s0 == "a" && obj.s9 == "j";
}
static_assert(test_mixed_fields_30(), "Struct with 30 mixed-type fields");

// Test 3: Struct with nested structs (many fields total)
struct NestedLevel2 {
    int a, b, c, d, e;
};

struct NestedLevel1 {
    NestedLevel2 n1, n2, n3, n4, n5;
    int x, y, z;
};

struct NestedManyFields {
    NestedLevel1 level1;
    int root_field;
};

constexpr bool test_nested_many_fields() {
    NestedManyFields obj{};
    constexpr std::string_view json {R"({"level1":{"n1":{"a":1,"b":2,"c":3,"d":4,"e":5},"n2":{"a":10,"b":20,"c":30,"d":40,"e":50},"n3":{"a":100,"b":200,"c":300,"d":400,"e":500},"n4":{"a":1000,"b":2000,"c":3000,"d":4000,"e":5000},"n5":{"a":10000,"b":20000,"c":30000,"d":40000,"e":50000},"x":999,"y":888,"z":777},"root_field":42})"};
    
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    
    return obj.level1.n1.a == 1
        && obj.level1.n5.e == 50000
        && obj.level1.x == 999
        && obj.root_field == 42;
}
static_assert(test_nested_many_fields(), "Nested structs with many fields total");

// Test 4: Struct with arrays (many fields via arrays)
struct ManyArrayFields {
    std::array<int, 10> arr0, arr1, arr2, arr3, arr4;
    std::array<bool, 5> flags0, flags1, flags2;
};

constexpr bool test_many_array_fields() {
    ManyArrayFields obj{};
    constexpr std::string_view json {R"({"arr0":[0,1,2,3,4,5,6,7,8,9],"arr1":[10,11,12,13,14,15,16,17,18,19],"arr2":[20,21,22,23,24,25,26,27,28,29],"arr3":[30,31,32,33,34,35,36,37,38,39],"arr4":[40,41,42,43,44,45,46,47,48,49],"flags0":[true,false,true,false,true],"flags1":[false,true,false,true,false],"flags2":[true,true,false,false,true]})"};
    
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    
    return obj.arr0[0] == 0 && obj.arr0[9] == 9
        && obj.arr4[0] == 40 && obj.arr4[9] == 49
        && obj.flags0[0] == true && obj.flags0[4] == true
        && obj.flags2[2] == false;
}
static_assert(test_many_array_fields(), "Struct with many array fields");

