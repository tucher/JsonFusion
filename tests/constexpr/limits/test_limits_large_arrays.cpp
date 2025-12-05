#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <array>
#include <string>

using namespace TestHelpers;
using namespace JsonFusion;

// ============================================================================
// Test: Large Array Limits
// ============================================================================

// Test 1: Large array of integers (100 elements - reduced for constexpr)
constexpr bool test_large_array_int() {
    std::array<int, 100> arr{};
    // Build JSON string manually (constexpr-compatible)
    constexpr std::string_view json {R"([0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99])"};
    
    auto result = JsonFusion::Parse(arr, json);
    
    if (!result) return false;
    
    // Verify first, middle, and last elements
    return arr[0] == 0
        && arr[50] == 50
        && arr[99] == 99;
}
static_assert(test_large_array_int(), "Large array of integers (100 elements)");

// Test 2: Large array of booleans (50 elements)
constexpr bool test_large_array_bool() {
    std::array<bool, 50> arr{};
    // Pattern: true, false, true, false, ...
    constexpr std::string_view json {R"([true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false,true,false])"};
    
    auto result = JsonFusion::Parse(arr, json);
    
    if (!result) return false;
    
    // Verify pattern
    return arr[0] == true
        && arr[1] == false
        && arr[49] == false;
}
static_assert(test_large_array_bool(), "Large array of booleans (50 elements)");

// Test 3: Large array of char arrays (20 elements)
struct LargeStringArray {
    std::array<std::array<char, 16>, 20> strings;
};

constexpr bool test_large_array_strings() {
    LargeStringArray obj{};
    constexpr std::string_view json {R"({"strings":["str0","str1","str2","str3","str4","str5","str6","str7","str8","str9","str10","str11","str12","str13","str14","str15","str16","str17","str18","str19"]})"};
    
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    
    // Verify first and last strings
    std::string_view first(obj.strings[0].data(), 4);
    std::string_view last(obj.strings[19].data(), 5);
    return first == "str0" && last == "str19";
}
static_assert(test_large_array_strings(), "Large array of strings (20 elements)");

// Test 4: Large nested arrays (2D: 10x10 = 100 elements)
constexpr bool test_large_2d_array() {
    std::array<std::array<int, 10>, 10> matrix{};
    constexpr std::string_view json {R"([[0,1,2,3,4,5,6,7,8,9],[10,11,12,13,14,15,16,17,18,19],[20,21,22,23,24,25,26,27,28,29],[30,31,32,33,34,35,36,37,38,39],[40,41,42,43,44,45,46,47,48,49],[50,51,52,53,54,55,56,57,58,59],[60,61,62,63,64,65,66,67,68,69],[70,71,72,73,74,75,76,77,78,79],[80,81,82,83,84,85,86,87,88,89],[90,91,92,93,94,95,96,97,98,99]])"};
    
    auto result = JsonFusion::Parse(matrix, json);
    
    if (!result) return false;
    
    // Verify corners and center
    return matrix[0][0] == 0
        && matrix[0][9] == 9
        && matrix[9][0] == 90
        && matrix[9][9] == 99
        && matrix[5][5] == 55;
}
static_assert(test_large_2d_array(), "Large 2D array (10x10 = 100 elements)");

// Test 5: Large array in struct (50 elements)
struct WithLargeArray {
    int id;
    std::array<int, 50> data;
    bool active;
};

constexpr bool test_large_array_in_struct() {
    WithLargeArray obj{};
    constexpr std::string_view json {R"({"id":42,"data":[0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,98],"active":true})"};
    
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    
    return obj.id == 42
        && obj.data[0] == 0
        && obj.data[25] == 50
        && obj.data[49] == 98
        && obj.active == true;
}
static_assert(test_large_array_in_struct(), "Large array in struct (50 elements)");

