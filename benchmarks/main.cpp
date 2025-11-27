#include <iostream>
#include <format>
#include <cassert>
#include <chrono>
#include <string_view>

#include <JsonFusion/parser.hpp>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <rapidjson/reader.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/error/en.h>


#include "../tests/test_model.hpp"
#include "bench_matrix.hpp"


namespace json_fusion_benchmarks {
using namespace JsonFusion::validators;
template<class T, class ... O>
using A = JsonFusion::Annotated<T, O...>;

using std::array, std::optional, std::list, std::vector, std::string;

constexpr std::size_t kMult = 16;
using SmallStr  = array<char, 16 * kMult>;
using MediumStr = array<char, 32 * kMult>;
using LargeStr  = array<char, 64 * kMult>;



struct EmbeddedConfigSmall {
    static constexpr std::string_view name = "EmbeddedConfig/small";


    struct EmbeddedConfigStatic {
        static constexpr std::size_t kMaxMotors  = 16;
        static constexpr std::size_t kMaxSensors = 16;

        MediumStr         app_name;
        uint16_t          version_major;
        int               version_minor;

        struct Network {
            SmallStr name;
            SmallStr address; // e.g. "192.168.0.1/24"
            uint16_t port;
            bool     enabled;
        };

        Network           network;
        optional<Network> fallback_network_conf;

        struct Controller {
            MediumStr name;
            A<int, range<10, 10000>> loop_hz;


            struct Motor {
                int64_t  id;
                SmallStr name;

                // [x,y,z]
                A<
                    array<A<double, range<-1000, 1000>>, 3>,
                    min_items<3>
                    > position;

                // [vx,vy,vz]
                A<
                    array<A<float, range<-1000, 1000>>, 3>,
                    min_items<3>
                    > vel_limits;

                bool inverted;
            };

            A<array<Motor,  kMaxMotors>,  min_items<1>> motors;

            struct Sensor {
                SmallStr  type;
                MediumStr model;
                A<float,  range<-100,  100000>> range_min;
                A<double, range<-1000, 100000>> range_max;
                bool      active;
            };

            A<array<Sensor, kMaxSensors>, min_items<1>> sensors;
        };

        Controller        controller;

        struct Logging {
            bool      enabled;
            LargeStr  path;
            uint32_t  max_files;
        };

        Logging           logging;
    };

    struct EmbeddedConfigDynamic {
        string         app_name;
        uint16_t            version_major;
        int                 version_minor;

        struct Network {
            string name;
            string address;
            uint16_t    port;
            bool        enabled;
        };


        Network             network;
        optional<Network> fallback_network_conf;

        struct Controller {
            struct Motor {
                int64_t              id;
                string          name;
                A<vector<A<double, range<-1000, 1000>>>, min_items<3>>
                    position;   // [x,y,z]
                A<vector<A<float,  range<-1000, 1000>>>, min_items<3>>
                    vel_limits; // [vx,vy,vz]
                bool                 inverted;
            };

            struct Sensor {
                string type;
                string model;
                A<float,  range<-100,  100000>> range_min;
                A<double, range<-1000, 100000>> range_max;
                bool      active;
            };

            string name;
            A<int, range<10, 10000>> loop_hz;
            A<vector<Motor>,  min_items<1>> motors;
            A<vector<Sensor>, min_items<1>> sensors;
        };

        Controller          controller;

        struct Logging {
            bool        enabled;
            string path;
            uint32_t    max_files;
        };

        Logging             logging;
    };




    using StaticModel  = EmbeddedConfigStatic;
    using DynamicModel = EmbeddedConfigDynamic;

    static constexpr std::size_t iter_count = 10000;

    // choose one default "json" – we can add more fields if needed
    static constexpr std::string_view json = R"JSON(
    {
      "app_name": "MotorCtrl-Embedded",
      "version_major": 1,
      "version_minor": 0,
      "network": {
        "name": "eth0",
        "address": "192.168.1.10/24",
        "port": 5020,
        "enabled": true
      },
      "fallback_network_conf": null,
      "controller": {
        "name": "main_controller",
        "loop_hz": 1000,
        "motors": [
          {
            "id": 1,
            "name": "X1",
            "position": [1.0, 2.0, 3.0],
            "vel_limits": [10.0, 10.0, 10.0],
            "inverted": false
          }
        ],
        "sensors": [
          {
            "type": "imu",
            "model": "IMU-9000",
            "range_min": -3.14,
            "range_max": 3.14,
            "active": true
          }
        ]
      },
      "logging": {
        "enabled": true,
        "path": "/var/log/motorctrl",
        "max_files": 8
      }
    }
    )JSON";
};



// One unversal parser for all
struct JF {
    static constexpr std::string_view library_name = "JsonFusion";

    template <class Model>
    bool parse_validate_and_populate(Model& out, const std::string& data, bool insitu /*not needed*/, std::string& remark) {
        auto res = JsonFusion::Parse(out, data);
        if (!res) {
            remark = "failed";
            return false;
        }
        return true;
    }

};


struct RapidJSON {
    static constexpr std::string_view library_name = "RapidJSON";


    rapidjson::Document doc;
    template <class Model>
    bool parse_validate_and_populate(Model& m, std::string& data, bool insitu, std::string& remark) {
        if(!insitu) {
            std::string buf = data;
            doc.ParseInsitu(buf.data());
        } else {
            doc.ParseInsitu(data.data());
        }

        if (doc.HasParseError()) {
            remark = std::string("Parse error: ") +
                     rapidjson::GetParseError_En(doc.GetParseError());
            return false;
        }
        remark = "Validation and population is not imlemented, DOM parsing only";
        return true;
    }
    template <class Model>
    bool parse_non_insitu(Model& out, const std::string& data, std::string& remark) {
        doc.Parse(data.data(), data.size());
        if (doc.HasParseError()) {
            return false;
        }
        return true;
    }

        // Not supported for this adapter – just omit parse_non_insitu
        // bool parse_non_insitu(...); // not declared

        // No mapping phase here – we don't populate Out
    template <class Model>
    bool validate_and_populate(Model&, std::string& remark) {
        remark = "not implemented";
        return false;
    }

};

}

using LibsToTest    = Libraries<json_fusion_benchmarks::JF, json_fusion_benchmarks::RapidJSON>;
using ConfigsToTest = Configs<json_fusion_benchmarks::EmbeddedConfigSmall>;

int main() {
    return run<LibsToTest, ConfigsToTest>();
}
