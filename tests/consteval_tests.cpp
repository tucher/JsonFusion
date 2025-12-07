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

using C = char[100];
static_assert(std::ranges::contiguous_range<C> &&
              std::same_as<std::ranges::range_value_t<C>, char>);


using namespace JsonFusion;



struct Motor {
    double position[3];
    bool active;
    char name[20];
    float transform[2][4][4];
};

template<>
struct JsonFusion::StructMeta<Motor> {
    using Fields = StructFields<
        Field<&Motor::position, "position", min_items<3>>,         // double position[3];
        Field<&Motor::active, "active">,                      //without annotations!  // bool active;
        Field<&Motor::name, "name",  min_length<3>>,   // char name[20];
        Field<&Motor::transform, "transform">
    >;
};


struct Motor2 {
    int64_t position[3];
    bool active;
    char name[20];
};

template<>
struct JsonFusion::StructMeta<Motor2> {
    using Fields = StructFields<
        Field<&Motor2::position, "position", min_items<3>>,  // int64_t position[3];
        Field<&Motor2::active, "active">,                     // bool active;
        Field<&Motor2::name, "name", min_length<1>>           // char name[20];
        >;
};

struct MotorSystem {
    Motor2 primary_motor;           // Nested Motor
    Motor2 motors[5];                // C array of Motors
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

int main() {

    {

        Motor test;
        memset(&test, 0, sizeof(Motor));

        std::string_view json (R"({
            "active": true,
            "position": [0.1, 0.2, 0.3],
            "transform": [
                [
                    [1,2,3,4],
                    [5,6,7,8],
                    [9,0,1,2],
                    [3,4,5,6]
                ],
                [
                    [1,2,3,4],
                    [5,6,7,8],
                    [9,0,1,2],
                    [3,4,5,6]
                ]
            ],
            "name": "Some stwefwefwefewf"
        })");
        auto r = JsonFusion::Parse(test, json);
        if(!r) {
            std::cerr << ParseResultToString<Motor>(r, json.begin(), json.end()) << std::endl;

        }
        assert(r);

        std::string out;
        assert(JsonFusion::Serialize(test, out));

        std::cout << out << std::endl;

    }
    {
        MotorSystem system1{};

        // Setup primary motor
        system1.primary_motor.position[0] = 1;
        system1.primary_motor.position[1] = 2;
        system1.primary_motor.position[2] = 3;
        system1.primary_motor.active = true;
        cstr_copy(system1.primary_motor.name, "Primary");

        // Setup motors array
        system1.motors[0].position[0] = 10;
        system1.motors[0].position[1] = 20;
        system1.motors[0].position[2] = 30;
        system1.motors[0].active = true;
        cstr_copy(system1.motors[0].name, "Motor1");

        system1.motor_count = 1;
        cstr_copy(system1.system_name, "RoundTrip");

        std::string serialized;
        if (!Serialize(system1, serialized)) return false;

        MotorSystem system2{};
        if (auto r = Parse(system2, serialized.data(), serialized.data() + serialized.size()); !r) {
            std::cerr << ParseResultToString<MotorSystem>(r, serialized.data(), serialized.data() + serialized.size()) << std::endl;
            return false;
        }

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

        return true;
    }

}
