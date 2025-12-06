#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/validators.hpp>
#include <JsonFusion/options.hpp>
#include <JsonFusion/struct_introspection.hpp>
#include <algorithm>
#include <cstdint>

using namespace TestHelpers;
using namespace JsonFusion;
using namespace JsonFusion::validators;
using namespace JsonFusion::options;

// ============================================================================
// Constexpr-safe string helpers for C arrays
// ============================================================================

// Constexpr string comparison for C arrays
template<std::size_t N>
constexpr bool cstr_equal(const char (&a)[N], const char* b) {
    std::size_t i = 0;
    while (i < N && a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return false;
        ++i;
    }
    return a[i] == '\0' && b[i] == '\0';
}

// Constexpr string copy for C arrays
template<std::size_t N>
constexpr void cstr_copy(char (&dest)[N], const char* src) {
    std::size_t i = 0;
    while (i < N - 1 && src[i] != '\0') {
        dest[i] = src[i];
        ++i;
    }
    dest[i] = '\0';  // Always null-terminate
}

// Constexpr string length
template<std::size_t N>
constexpr std::size_t cstr_len(const char (&str)[N]) {
    std::size_t i = 0;
    while (i < N && str[i] != '\0') {
        ++i;
    }
    return i;
}

// ============================================================================
// Motor Structure - Pure C struct with C arrays
// ============================================================================

struct Motor {
    int64_t position[3];
    bool active;
    char name[20];
};

// External annotation for Motor using StructMeta (required for C arrays)
template<>
struct JsonFusion::StructMeta<Motor> {
    using Fields = StructFields<
        Field<&Motor::position, "position", min_items<3>>,  // int64_t position[3];
        Field<&Motor::active, "active">,                     // bool active;
        Field<&Motor::name, "name", min_length<1>>           // char name[20];
    >;
};

// ============================================================================
// Test: Motor - Basic Parsing and Serialization
// ============================================================================

// Test: Parse Motor with all fields
constexpr bool test_motor_basic_parse() {
    Motor motor{};  // Zero-initialized
    
    std::string json = R"({
        "position": [1, 2, 3],
        "active": true,
        "name": "Motor1"
    })";
    
    auto result = Parse(motor, json);
    if (!result) return false;
    
    // Verify position array
    if (motor.position[0] != 1 || motor.position[1] != 2 || motor.position[2] != 3) {
        return false;
    }
    
    // Verify active
    if (!motor.active) return false;
    
    // Verify name (null-terminated C string)
    if (!cstr_equal(motor.name, "Motor1")) return false;
    
    return true;
}
static_assert(test_motor_basic_parse(), "Motor - basic parse");

// Test: Serialize Motor
constexpr bool test_motor_basic_serialize() {
    Motor motor{};
    motor.position[0] = 10;
    motor.position[1] = 20;
    motor.position[2] = 30;
    motor.active = false;
    cstr_copy(motor.name, "TestMotor");
    
    std::string result;
    if (!Serialize(motor, result)) return false;
    
    // Should contain all fields
    if (result.find("\"position\"") == std::string::npos) return false;
    if (result.find("\"active\"") == std::string::npos) return false;
    if (result.find("\"name\"") == std::string::npos) return false;
    if (result.find("TestMotor") == std::string::npos) return false;
    
    return true;
}
static_assert(test_motor_basic_serialize(), "Motor - basic serialize");

// Test: Round-trip Motor
constexpr bool test_motor_roundtrip() {
    Motor motor1{};
    motor1.position[0] = 1;
    motor1.position[1] = 2;
    motor1.position[2] = 3;
    motor1.active = true;
    cstr_copy(motor1.name, "RoundTrip");
    
    std::string serialized;
    if (!Serialize(motor1, serialized)) return false;
    
    Motor motor2{};
    if (!Parse(motor2, serialized)) return false;
    
    // Compare
    if (motor1.position[0] != motor2.position[0] ||
        motor1.position[1] != motor2.position[1] ||
        motor1.position[2] != motor2.position[2]) {
        return false;
    }
    if (motor1.active != motor2.active) return false;
    if (!cstr_equal(motor1.name, motor2.name)) return false;
    
    return true;
}
static_assert(test_motor_roundtrip(), "Motor - roundtrip");

// ============================================================================
// Test: Motor - C Array Edge Cases
// ============================================================================

// Test: Empty array (should fail validation due to min_items<3>)
constexpr bool test_motor_array_empty() {
    Motor motor{};
    
    std::string json = R"({
        "position": [],
        "active": true,
        "name": "Test"
    })";
    
    auto result = Parse(motor, json);
    // Should fail validation (min_items<3> requires at least 3 items)
    return !result;
}
static_assert(test_motor_array_empty(), "Motor - empty array should fail");

// Test: Array with too few elements (should fail validation)
constexpr bool test_motor_array_too_short() {
    Motor motor{};
    
    std::string json = R"({
        "position": [1, 2],
        "active": true,
        "name": "Test"
    })";
    
    auto result = Parse(motor, json);
    // Should fail validation (min_items<3> requires at least 3 items)
    return !result;
}
static_assert(test_motor_array_too_short(), "Motor - array too short should fail");

// Test: Array with exact size (should succeed)
constexpr bool test_motor_array_exact_size() {
    Motor motor{};
    
    std::string json = R"({
        "position": [1, 2, 3],
        "active": true,
        "name": "Test"
    })";
    
    auto result = Parse(motor, json);
    if (!result) return false;
    
    // Verify all 3 elements are set
    return motor.position[0] == 1 && 
           motor.position[1] == 2 && 
           motor.position[2] == 3;
}
static_assert(test_motor_array_exact_size(), "Motor - array exact size should succeed");

// Test: Array overflow (more elements than array size - should fail)
constexpr bool test_motor_array_overflow() {
    Motor motor{};
    
    std::string json = R"({
        "position": [1, 2, 3, 4],
        "active": true,
        "name": "Test"
    })";
    
    auto result = Parse(motor, json);
    // Should fail with overflow error (array size is 3, got 4)
    return !result;
}
static_assert(test_motor_array_overflow(), "Motor - array overflow should fail");

// Test: Array with proper size (3 elements, matches array size)
constexpr bool test_motor_array_proper_size() {
    Motor motor{};
    
    std::string json = R"({
        "position": [10, 20, 30],
        "active": false,
        "name": "ProperSize"
    })";
    
    auto result = Parse(motor, json);
    if (!result) return false;
    
    // Verify values
    if (motor.position[0] != 10 || 
        motor.position[1] != 20 || 
        motor.position[2] != 30) {
        return false;
    }
    if (motor.active != false) return false;
    if (!cstr_equal(motor.name, "ProperSize")) return false;
    
    return true;
}
static_assert(test_motor_array_proper_size(), "Motor - array proper size should succeed");

// ============================================================================
// Test: Motor - String Array Edge Cases
// ============================================================================

// Test: Empty string in name array
constexpr bool test_motor_name_empty() {
    Motor motor{};
    
    std::string json = R"({
        "position": [1, 2, 3],
        "active": true,
        "name": ""
    })";
    
    auto result = Parse(motor, json);
    // Should fail validation (min_length<1> requires at least 1 character)
    return !result;
}
static_assert(test_motor_name_empty(), "Motor - empty name should fail");

// Test: Name exactly filling buffer (19 chars + null terminator = 20)
constexpr bool test_motor_name_max_length() {
    Motor motor{};
    
    std::string json = R"({
        "position": [1, 2, 3],
        "active": true,
        "name": "1234567890123456789"
    })";
    
    auto result = Parse(motor, json);
    if (!result) return false;
    
    // Should be null-terminated
    return cstr_len(motor.name) == 19;
}
static_assert(test_motor_name_max_length(), "Motor - name max length should succeed");

// Test: Name overflow (too long for buffer)
constexpr bool test_motor_name_overflow() {
    Motor motor{};
    
    std::string json = R"({
        "position": [1, 2, 3],
        "active": true,
        "name": "This is a very long name that exceeds the buffer size of 20 characters"
    })";
    
    auto result = Parse(motor, json);
    // Should fail with overflow error
    return !result;
}
static_assert(test_motor_name_overflow(), "Motor - name overflow should fail");

// ============================================================================
// MotorSystem Structure - Nested Motor and C Array of Motors
// ============================================================================

struct MotorSystem {
    Motor primary_motor;           // Nested Motor
    Motor motors[5];                // C array of Motors
    int motor_count;
    char system_name[32];
};

// External annotation for MotorSystem
template<>
struct JsonFusion::StructMeta<MotorSystem> {
    using Fields = StructFields<
        Field<&MotorSystem::primary_motor, "primary_motor">,      // Nested Motor
        Field<&MotorSystem::motors, "motors", max_items<5>>,       // C array of Motors
        Field<&MotorSystem::motor_count, "motor_count">,          // int
        Field<&MotorSystem::system_name, "system_name", min_length<1>, max_length<31>>  // char[32]
    >;
};

// ============================================================================
// Test: MotorSystem - Basic Parsing and Serialization
// ============================================================================

// Test: Parse MotorSystem with nested Motor and array of Motors
constexpr bool test_motorsystem_basic_parse() {
    MotorSystem system{};
    
    std::string json = R"({
        "primary_motor": {
            "position": [1, 2, 3],
            "active": true,
            "name": "Primary"
        },
        "motors": [
            {
                "position": [10, 20, 30],
                "active": true,
                "name": "Motor1"
            },
            {
                "position": [11, 21, 31],
                "active": false,
                "name": "Motor2"
            }
        ],
        "motor_count": 2,
        "system_name": "TestSystem"
    })";
    
    auto result = Parse(system, json);
    if (!result) return false;
    
    // Verify primary_motor
    if (system.primary_motor.position[0] != 1 ||
        system.primary_motor.position[1] != 2 ||
        system.primary_motor.position[2] != 3) {
        return false;
    }
    if (!system.primary_motor.active) return false;
    if (!cstr_equal(system.primary_motor.name, "Primary")) return false;
    
    // Verify first motor in array
    if (system.motors[0].position[0] != 10 ||
        system.motors[0].position[1] != 20 ||
        system.motors[0].position[2] != 30) {
        return false;
    }
    if (!system.motors[0].active) return false;
    if (!cstr_equal(system.motors[0].name, "Motor1")) return false;
    
    // Verify second motor in array
    if (system.motors[1].position[0] != 11 ||
        system.motors[1].position[1] != 21 ||
        system.motors[1].position[2] != 31) {
        return false;
    }
    if (system.motors[1].active != false) return false;
    if (!cstr_equal(system.motors[1].name, "Motor2")) return false;
    
    // Verify motor_count
    if (system.motor_count != 2) return false;
    
    // Verify system_name
    if (!cstr_equal(system.system_name, "TestSystem")) return false;
    
    return true;
}
static_assert(test_motorsystem_basic_parse(), "MotorSystem - basic parse");

// Test: Serialize MotorSystem
constexpr bool test_motorsystem_basic_serialize() {
    MotorSystem system{};
    
    // Setup primary motor
    system.primary_motor.position[0] = 1;
    system.primary_motor.position[1] = 2;
    system.primary_motor.position[2] = 3;
    system.primary_motor.active = true;
    cstr_copy(system.primary_motor.name, "Primary");
    
    // Setup first motor in array
    system.motors[0].position[0] = 10;
    system.motors[0].position[1] = 20;
    system.motors[0].position[2] = 30;
    system.motors[0].active = true;
    cstr_copy(system.motors[0].name, "Motor1");
    
    system.motor_count = 1;
    cstr_copy(system.system_name, "TestSystem");
    
    std::string result;
    if (!Serialize(system, result)) return false;
    
    // Should contain all fields
    if (result.find("\"primary_motor\"") == std::string::npos) return false;
    if (result.find("\"motors\"") == std::string::npos) return false;
    if (result.find("\"motor_count\"") == std::string::npos) return false;
    if (result.find("\"system_name\"") == std::string::npos) return false;
    if (result.find("Primary") == std::string::npos) return false;
    if (result.find("Motor1") == std::string::npos) return false;
    if (result.find("TestSystem") == std::string::npos) return false;
    
    return true;
}
static_assert(test_motorsystem_basic_serialize(), "MotorSystem - basic serialize");

// ============================================================================
// Test: MotorSystem - C Array Edge Cases
// ============================================================================

// Test: Empty motors array (should succeed - empty array is valid)
constexpr bool test_motorsystem_array_empty() {
    MotorSystem system{};
    
    std::string json = R"({
        "primary_motor": {
            "position": [1, 2, 3],
            "active": true,
            "name": "Primary"
        },
        "motors": [],
        "motor_count": 0,
        "system_name": "Empty"
    })";
    
    auto result = Parse(system, json);
    if (!result) return false;
    
    // Verify motor_count
    if (system.motor_count != 0) return false;
    
    // Verify system_name
    if (!cstr_equal(system.system_name, "Empty")) return false;
    
    return true;
}
static_assert(test_motorsystem_array_empty(), "MotorSystem - empty motors array should succeed");

// Test: Motors array with exact max size (5 motors)
constexpr bool test_motorsystem_array_max_size() {
    MotorSystem system{};
    
    std::string json = R"({
        "primary_motor": {
            "position": [1, 2, 3],
            "active": true,
            "name": "Primary"
        },
        "motors": [
            {"position": [1, 2, 3], "active": true, "name": "M1"},
            {"position": [2, 3, 4], "active": true, "name": "M2"},
            {"position": [3, 4, 5], "active": true, "name": "M3"},
            {"position": [4, 5, 6], "active": true, "name": "M4"},
            {"position": [5, 6, 7], "active": true, "name": "M5"}
        ],
        "motor_count": 5,
        "system_name": "MaxSize"
    })";
    
    auto result = Parse(system, json);
    if (!result) return false;
    
    // Verify all 5 motors are set
    for (int i = 0; i < 5; ++i) {
        if (system.motors[i].position[0] != static_cast<int64_t>(i + 1) ||
            system.motors[i].position[1] != static_cast<int64_t>(i + 2) ||
            system.motors[i].position[2] != static_cast<int64_t>(i + 3)) {
            return false;
        }
        if (!system.motors[i].active) return false;
    }
    
    return true;
}
static_assert(test_motorsystem_array_max_size(), "MotorSystem - array max size should succeed");

// Test: Motors array overflow (more than 5 motors - should fail)
constexpr bool test_motorsystem_array_overflow() {
    MotorSystem system{};
    
    std::string json = R"({
        "primary_motor": {
            "position": [1, 2, 3],
            "active": true,
            "name": "Primary"
        },
        "motors": [
            {"position": [1, 2, 3], "active": true, "name": "M1"},
            {"position": [2, 3, 4], "active": true, "name": "M2"},
            {"position": [3, 4, 5], "active": true, "name": "M3"},
            {"position": [4, 5, 6], "active": true, "name": "M4"},
            {"position": [5, 6, 7], "active": true, "name": "M5"},
            {"position": [6, 7, 8], "active": true, "name": "M6"}
        ],
        "motor_count": 6,
        "system_name": "Overflow"
    })";
    
    auto result = Parse(system, json);
    // Should fail with overflow error (array size is 5, got 6)
    return !result;
}
static_assert(test_motorsystem_array_overflow(), "MotorSystem - array overflow should fail");

// Test: Motors array with proper size (less than max)
constexpr bool test_motorsystem_array_proper_size() {
    MotorSystem system{};
    
    std::string json = R"({
        "primary_motor": {
            "position": [1, 2, 3],
            "active": true,
            "name": "Primary"
        },
        "motors": [
            {"position": [10, 20, 30], "active": true, "name": "Motor1"},
            {"position": [11, 21, 31], "active": false, "name": "Motor2"}
        ],
        "motor_count": 2,
        "system_name": "ProperSize"
    })";
    
    auto result = Parse(system, json);
    if (!result) return false;
    
    // Verify first motor
    if (system.motors[0].position[0] != 10 ||
        system.motors[0].position[1] != 20 ||
        system.motors[0].position[2] != 30) {
        return false;
    }
    if (!system.motors[0].active) return false;
    if (!cstr_equal(system.motors[0].name, "Motor1")) return false;
    
    // Verify second motor
    if (system.motors[1].position[0] != 11 ||
        system.motors[1].position[1] != 21 ||
        system.motors[1].position[2] != 31) {
        return false;
    }
    if (system.motors[1].active != false) return false;
    if (!cstr_equal(system.motors[1].name, "Motor2")) return false;
    
    // Verify motor_count
    if (system.motor_count != 2) return false;
    
    // Verify system_name
    if (!cstr_equal(system.system_name, "ProperSize")) return false;
    
    return true;
}
static_assert(test_motorsystem_array_proper_size(), "MotorSystem - array proper size should succeed");

// ============================================================================
// Test: MotorSystem - System Name Edge Cases
// ============================================================================

// Test: System name empty (should fail validation)
constexpr bool test_motorsystem_name_empty() {
    MotorSystem system{};
    
    std::string json = R"({
        "primary_motor": {
            "position": [1, 2, 3],
            "active": true,
            "name": "Primary"
        },
        "motors": [],
        "motor_count": 0,
        "system_name": ""
    })";
    
    auto result = Parse(system, json);
    // Should fail validation (min_length<1> requires at least 1 character)
    return !result;
}
static_assert(test_motorsystem_name_empty(), "MotorSystem - empty system_name should fail");

// Test: System name max length (31 chars + null terminator = 32)
constexpr bool test_motorsystem_name_max_length() {
    MotorSystem system{};
    
    std::string json = R"({
        "primary_motor": {
            "position": [1, 2, 3],
            "active": true,
            "name": "Primary"
        },
        "motors": [],
        "motor_count": 0,
        "system_name": "1234567890123456789012345678901"
    })";
    
    auto result = Parse(system, json);
    if (!result) return false;
    
    // Should be null-terminated and exactly 31 characters
    return cstr_len(system.system_name) == 31;
}
static_assert(test_motorsystem_name_max_length(), "MotorSystem - system_name max length should succeed");

// Test: System name overflow (too long for buffer)
constexpr bool test_motorsystem_name_overflow() {
    MotorSystem system{};
    
    std::string json = R"({
        "primary_motor": {
            "position": [1, 2, 3],
            "active": true,
            "name": "Primary"
        },
        "motors": [],
        "motor_count": 0,
        "system_name": "This is a very long system name that exceeds the buffer size of 32 characters"
    })";
    
    auto result = Parse(system, json);
    // Should fail with overflow error
    return !result;
}
static_assert(test_motorsystem_name_overflow(), "MotorSystem - system_name overflow should fail");

// ============================================================================
// Test: C Array Serialization Issue - Demonstrates API Usage Problem
// ============================================================================

// Test: Demonstrates the problem - uninitialized array elements fail validation
// This test shows what happens when you serialize a C array with uninitialized elements
// that don't meet validation constraints. This highlights the fundamental issue:
// C arrays serialize ALL elements, but validation applies to ALL elements.
constexpr bool test_motorsystem_serialization_problem() {
    MotorSystem system{};
    
    // Only initialize first motor
    system.motors[0].position[0] = 10;
    system.motors[0].position[1] = 20;
    system.motors[0].position[2] = 30;
    system.motors[0].active = true;
    cstr_copy(system.motors[0].name, "Motor1");
    
    // motors[1-4] are zero-initialized (empty names)
    // When serialized, these empty names violate min_length<1> constraint
    
    system.motor_count = 1;
    cstr_copy(system.system_name, "Test");
    
    // Setup primary motor
    system.primary_motor.position[0] = 1;
    system.primary_motor.position[1] = 2;
    system.primary_motor.position[2] = 3;
    system.primary_motor.active = true;
    cstr_copy(system.primary_motor.name, "Primary");
    
    std::string serialized;
    if (!Serialize(system, serialized)) return false;
    
    // Now try to parse it back - this will fail because motors[1-4] have empty names
    MotorSystem system2{};
    auto result = Parse(system2, serialized);
    
    // This should fail because uninitialized motors have empty names (min_length<1> violation)
    return !result;
}
static_assert(test_motorsystem_serialization_problem(), "MotorSystem - demonstrates serialization problem with uninitialized elements");

// ============================================================================
// Test: Round-trip Tests
// ============================================================================

// Test: Round-trip MotorSystem
// IMPORTANT API USAGE NOTE:
// C arrays serialize ALL elements, not just "used" ones. When serializing MotorSystem,
// all 5 motors in the array are serialized, including uninitialized ones.
// Since Motor.name has min_length<1> validation, ALL motors must have valid names,
// even if they're "unused". This highlights a fundamental mismatch:
// - C arrays: Fixed-size, all elements always present
// - JSON arrays: Variable-size, only meaningful elements
// - Validation: Applies to all elements
// Solutions:
// 1. Initialize all array elements with valid default values
// 2. Use std::vector with a count field instead of C array
// 3. Make validation conditional (e.g., remove min_length for unused elements)
constexpr bool test_motorsystem_roundtrip() {
    MotorSystem system1{};
    
    // Setup primary motor
    system1.primary_motor.position[0] = 1;
    system1.primary_motor.position[1] = 2;
    system1.primary_motor.position[2] = 3;
    system1.primary_motor.active = true;
    cstr_copy(system1.primary_motor.name, "Primary");
    
    // Setup motors array - MUST initialize all motors with valid names
    // because C arrays serialize all elements and validation applies to all
    system1.motors[0].position[0] = 10;
    system1.motors[0].position[1] = 20;
    system1.motors[0].position[2] = 30;
    system1.motors[0].active = true;
    cstr_copy(system1.motors[0].name, "Motor1");
    
    // Initialize remaining motors with valid default values to satisfy validation
    // (min_length<1> requires non-empty names for all motors in the array)
    // C arrays serialize ALL elements, so all must meet validation constraints
    system1.motors[1].position[0] = 0;
    system1.motors[1].position[1] = 0;
    system1.motors[1].position[2] = 0;
    system1.motors[1].active = false;
    cstr_copy(system1.motors[1].name, "Unused");
    
    system1.motors[2].position[0] = 0;
    system1.motors[2].position[1] = 0;
    system1.motors[2].position[2] = 0;
    system1.motors[2].active = false;
    cstr_copy(system1.motors[2].name, "Unused");
    
    system1.motors[3].position[0] = 0;
    system1.motors[3].position[1] = 0;
    system1.motors[3].position[2] = 0;
    system1.motors[3].active = false;
    cstr_copy(system1.motors[3].name, "Unused");
    
    system1.motors[4].position[0] = 0;
    system1.motors[4].position[1] = 0;
    system1.motors[4].position[2] = 0;
    system1.motors[4].active = false;
    cstr_copy(system1.motors[4].name, "Unused");
    
    system1.motor_count = 1;
    cstr_copy(system1.system_name, "RoundTrip");
    
    std::string serialized;
    if (!Serialize(system1, serialized)) return false;
    
    MotorSystem system2{};
    if (!Parse(system2, serialized)) return false;
    
    // Compare primary_motor
    if (system1.primary_motor.position[0] != system2.primary_motor.position[0] ||
        system1.primary_motor.position[1] != system2.primary_motor.position[1] ||
        system1.primary_motor.position[2] != system2.primary_motor.position[2]) {
        return false;
    }
    if (system1.primary_motor.active != system2.primary_motor.active) return false;
    if (!cstr_equal(system1.primary_motor.name, system2.primary_motor.name)) return false;
    
    // Compare first motor in array
    if (system1.motors[0].position[0] != system2.motors[0].position[0] ||
        system1.motors[0].position[1] != system2.motors[0].position[1] ||
        system1.motors[0].position[2] != system2.motors[0].position[2]) {
        return false;
    }
    if (system1.motors[0].active != system2.motors[0].active) return false;
    if (!cstr_equal(system1.motors[0].name, system2.motors[0].name)) return false;
    
    // Compare motor_count
    if (system1.motor_count != system2.motor_count) return false;
    
    // Compare system_name
    if (!cstr_equal(system1.system_name, system2.system_name)) return false;
    
    // Note: We don't compare motors[1-4] because they're "unused" placeholders
    // with default values. In a real application, you'd use motor_count to
    // determine which motors are meaningful.
    
    return true;
}
static_assert(test_motorsystem_roundtrip(), "MotorSystem - roundtrip");

// ============================================================================
// All C Interop Tests Pass!
// ============================================================================

constexpr bool all_c_interop_tests_pass = true;
static_assert(all_c_interop_tests_pass, "[[[ All C interop tests must pass at compile time ]]]");

int main() {
    // All tests run at compile time via static_assert
    return 0;
}
