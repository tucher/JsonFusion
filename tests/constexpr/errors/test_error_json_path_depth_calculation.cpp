#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/static_schema.hpp>
#include <array>

using namespace JsonFusion;
using namespace JsonFusion::schema_analyzis;

// ============================================================================
// Test: Compile-Time Schema Depth Calculation (calc_type_depth)
// ============================================================================

// Test 1: Primitives have depth 1
struct Primitives {
    int x;
    bool flag;
    double value;
};

static_assert(calc_type_depth<int>() == 1, "Primitive int: depth = 1");
static_assert(calc_type_depth<bool>() == 1, "Primitive bool: depth = 1");
static_assert(calc_type_depth<double>() == 1, "Primitive double: depth = 1");
static_assert(calc_type_depth<Primitives>() == 2, "Flat struct: depth = 1 (struct) + 1 (primitives)");

// ============================================================================
// Arrays Increase Depth
// ============================================================================

struct WithArray {
    std::array<int, 3> values;
};

// Array of primitives: 1 (struct) + 1 (array) + 1 (int) = 3
static_assert(calc_type_depth<WithArray>() == 3, "Array of primitives: depth = 3");

struct With2DArray {
    std::array<std::array<int, 3>, 2> matrix;
};

// 2D array: 1 (struct) + 1 (outer array) + 1 (inner array) + 1 (int) = 4
static_assert(calc_type_depth<With2DArray>() == 4, "2D array: depth = 4");

struct With3DArray {
    std::array<std::array<std::array<int, 2>, 2>, 2> tensor;
};

// 3D array: 1 (struct) + 1 + 1 + 1 (arrays) + 1 (int) = 5
static_assert(calc_type_depth<With3DArray>() == 5, "3D array: depth = 5");

// ============================================================================
// Nested Structs Increase Depth
// ============================================================================

struct Inner1 {
    int value;
};

struct Outer1 {
    Inner1 inner;
};

// Nested struct: 1 (Outer) + 1 (Inner) + 1 (int) = 3
static_assert(calc_type_depth<Outer1>() == 3, "Nested struct (2 levels): depth = 3");

struct Level3 {
    int data;
};

struct Level2 {
    Level3 deep;
};

struct Level1 {
    Level2 middle;
};

// Deep nesting: 1 + 1 + 1 + 1 = 4
static_assert(calc_type_depth<Level1>() == 4, "Deep nesting (3 levels): depth = 4");

// ============================================================================
// Depth is Maximum of All Fields
// ============================================================================

struct MixedDepth {
    int shallow;                    // depth 1
    struct Inner { int x; } nested; // depth 2
    std::array<int, 3> arr;         // depth 2
};

// Depth is max(1, 2, 2) + 1 = 3
static_assert(calc_type_depth<MixedDepth>() == 3, "Mixed depth fields: depth = max + 1");

struct VariableDepth {
    int a;                                              // depth 1
    std::array<int, 2> b;                               // depth 2
    struct Deep { std::array<int, 2> values; } c;       // depth 3
};

// Depth is max(1, 2, 3) + 1 = 4
static_assert(calc_type_depth<VariableDepth>() == 4, "Variable depth fields: depth = max(fields) + 1");

// ============================================================================
// Optionals are Transparent (Don't Add Depth)
// ============================================================================

struct WithOptional {
    std::optional<int> value;
};

// Optional doesn't add depth: 1 (struct) + 1 (int) = 2
static_assert(calc_type_depth<WithOptional>() == 2, "Optional primitive: depth = 2 (no extra level)");

struct OptInner {
    int x;
};

struct WithNestedOptional {
    std::optional<OptInner> inner;
};

// Optional is transparent: 1 (struct) + 1 (OptInner) + 1 (int) = 3
static_assert(calc_type_depth<WithNestedOptional>() == 3, "Optional struct: depth = 3");

struct WithOptionalArray {
    std::optional<std::array<int, 3>> values;
};

// Optional doesn't add depth: 1 (struct) + 1 (array) + 1 (int) = 3
static_assert(calc_type_depth<WithOptionalArray>() == 3, "Optional array: depth = 3");

// ============================================================================
// Array of Structs
// ============================================================================

struct Point {
    int x;
    int y;
};

struct WithStructArray {
    std::array<Point, 3> points;
};

// Array of structs: 1 (outer) + 1 (array) + 1 (Point) + 1 (int) = 4
static_assert(calc_type_depth<WithStructArray>() == 4, "Array of structs: depth = 4");

// ============================================================================
// Complex Nested Combinations
// ============================================================================

struct Deep {
    int value;
};

struct Complex {
    struct Nested {
        std::array<Deep, 2> items;
    } data;
};

// Complex: 1 (Complex) + 1 (Nested) + 1 (array) + 1 (Deep) + 1 (int) = 5
static_assert(calc_type_depth<Complex>() == 5, "Complex nesting: depth = 5");

// ============================================================================
// Recursive Types Return SCHEMA_UNBOUNDED
// ============================================================================

// Recursive types can be defined using std::unique_ptr or std::vector to avoid
// incomplete type errors. calc_type_depth() detects recursion by tracking SeenTypes
// and returns SCHEMA_UNBOUNDED when a type appears in its own definition chain.
//
// CORRECT ways to define recursive types:
//
// Linked list with std::unique_ptr:
//   struct Node {
//       int value;
//       std::unique_ptr<Node> next;  // OK: unique_ptr doesn't need complete type
//   };
//
// Binary tree with std::unique_ptr:
//   struct TreeNode {
//       int data;
//       std::unique_ptr<TreeNode> left;
//       std::unique_ptr<TreeNode> right;
//   };
//
// Array of children (common pattern):
//   struct TreeNode {
//       int data;
//       std::vector<TreeNode> children;  // OK: vector handles incomplete types
//   };
//
// INCORRECT (causes incomplete type errors):
//   struct Node {
//       int value;
//       std::optional<Node> next;  // ERROR: optional needs complete type
//   };
//
// When calc_type_depth() returns SCHEMA_UNBOUNDED, JsonFusion:
// 1. Uses dynamic path storage (std::vector) instead of std::array
// 2. Requires JSONFUSION_ALLOW_JSON_PATH_STRING_ALLOCATION_FOR_MAP_ACCESS macro
// 3. Still provides full error reporting with JSON paths

// ============================================================================
// Edge Cases: not_json fields
// ============================================================================

struct OnlyNotJson {
    A<int, options::not_json> hidden;
    int dummy;  // At least one parsable field
};

// not_json fields are ignored in depth calculation
// depth = 1 (struct) + 1 (dummy primitive) = 2
static_assert(calc_type_depth<OnlyNotJson>() == 2, "Struct with not_json fields");

// ============================================================================
// Depth Calculation Impacts JsonPath Storage Size
// ============================================================================

// Verify that parser uses the calculated depth for path storage
struct FlatStruct {
    int x;
    int y;
};

// Flat struct (depth 2) should use smaller path storage than deep nested
constexpr std::size_t flat_depth = calc_type_depth<FlatStruct>();
constexpr std::size_t deep_depth = calc_type_depth<Level1>();

static_assert(flat_depth < deep_depth, "Flat struct has smaller depth than deeply nested");

// ============================================================================
// Real-World Schema Depths
// ============================================================================

// Typical config file structure
struct ServerConfig {
    int port;
    struct Database {
        std::array<char, 256> host;
        int port;
        std::array<char, 128> name;
    } db;
    struct Logging {
        std::array<char, 32> level;
        bool enabled;
    } logging;
};

// Realistic config: 1 (ServerConfig) + 1 (nested struct) + 1 (primitives) = 3
static_assert(calc_type_depth<ServerConfig>() == 3, "Typical server config: depth = 3");

// API response with arrays
struct ItemMeta {
    bool active;
};

struct Item {
    int id;
    std::array<char, 128> name;
    ItemMeta meta;
};

struct ApiResponse {
    int status;
    std::array<Item, 10> items;
};

// API response: 1 (ApiResponse) + 1 (array) + 1 (Item) + 1 (Meta) + 1 (bool) = 5
static_assert(calc_type_depth<ApiResponse>() == 5, "API response with nested arrays: depth = 5");

// ============================================================================
// Verify Depth Matches Actual Maximum Path Length
// ============================================================================

// For a struct with depth N, the maximum error path length should be N-1
// (since depth includes the root, but path doesn't count it)

struct DepthTest {
    struct L1 {
        struct L2 {
            int value;
        } l2;
    } l1;
};

constexpr std::size_t depth_test_depth = calc_type_depth<DepthTest>();
static_assert(depth_test_depth == 4, "DepthTest: depth = 4");

// Maximum path would be: "l1" -> "l2" -> "value" (3 elements)
// Depth = 4 means path storage size = depth - 1 = 3 (correct!)

