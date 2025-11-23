#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "parser.hpp"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <rapidjson/reader.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/error/en.h>



using std::array;
using std::int64_t;
using std::list;
using std::optional;
using std::string;
using std::string_view;
using std::vector;

namespace rj = rapidjson;


using namespace JSONReflection2::options;
using JSONReflection2::Annotated;

namespace static_model {

using SmallStr  = std::array<char, 16>;
using MediumStr = std::array<char, 32>;
using LargeStr  = std::array<char, 64>;

// ---------- Sub-structures ----------

// Network interface configuration
struct NetworkStatic {
    SmallStr   name;
    LargeStr   address; // e.g. "192.168.0.1/24"
    int        port;
    bool       enabled;
};

// Motor channel configuration
struct MotorStatic {
    int         id;
    SmallStr       name;
    Annotated<std::array<
        Annotated<float, range<-1000, 1000>>,3>, min_items<3>>      position;        // [x,y,z]
    Annotated<std::array<
        Annotated<float, range<-1000, 1000>>,3>, min_items<3>>    vel_limits;      // [vx,vy,vz]
    bool     inverted;
};

// Sensor configuration
struct SensorStatic {
    SmallStr    type;    
    MediumStr   model;
    Annotated<float, range<-100, 100000> >  range_min;
    float      range_max;
    bool       active;
};

// Controller-level configuration
struct ControllerStatic {
    MediumStr                             name;
    Annotated<int, range<200, 10000>>     loop_hz;
    Annotated<std::array<MotorStatic, 4>, min_items<1>>    motors;
    std::array<SensorStatic, 4>  sensors;
};

// Logging configuration
struct LoggingStatic {
    bool        enabled;
    LargeStr    path;
    int         max_files;
};

// Top-level config
struct StaticComplexConfig {
    MediumStr         app_name;
    int               version_major;
    int               version_minor;
    NetworkStatic     network;
    ControllerStatic  controller;
    LoggingStatic     logging;
};
}

namespace dynamic_model {

struct NetworkDynamic {
    string    name;
    string  address; // e.g. "192.168.0.1/24"
    int     port;
    bool    enabled;
};

// Motor channel configuration
struct MotorDynamic {
    int             id;
    string          name;
    Annotated<vector<
            Annotated<float, range<-1000, 1000>>
        >,  max_items<3>, min_items<3>>      position;  
    Annotated<vector<
            Annotated<float, range<-1000, 1000>>
        >,  max_items<3>, min_items<3>>    vel_limits; 
    bool        inverted;
};

// Sensor configuration
struct SensorDynamic {
    string  type;       // "lidar", "imu", ...
    string  model;
    Annotated<float, range<-100, 100000> >  range_min;
    float   range_max;
    bool    active;
};

// Controller-level configuration
struct ControllerDynamic {
    string                     name;
    Annotated<int, range<200, 10000>>     loop_hz;
    Annotated<list<MotorDynamic>,  min_items<1>,  max_items<4>>    motors;
    Annotated<list<SensorDynamic>, max_items<4>>   sensors;
};

// Logging configuration
struct LoggingDynamic {
    bool      enabled;
    string    path;
    int       max_files;
};

// Top-level config
struct DynamicComplexConfig {
    string             app_name;
    int                version_major;
    int                version_minor;
    NetworkDynamic     network;
    ControllerDynamic  controller;
    LoggingDynamic     logging;
};

}

static constexpr std::string_view kJsonStatic = R"JSON(
{
  "app_name": "StaticBenchApp",
  "version_major": 1,
  "version_minor": 42,

  "network": {
    "name": "eth0",
    "address": "192.168.0.10/24",
    "port": 5020,
    "enabled": true
  },

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
      },
      {
        "id": 2,
        "name": "Y1",
        "position": [4.0, 5.0, 6.0],
        "vel_limits": [9.5, 9.5, 9.5],
        "inverted": true
      },
      {
        "id": 3,
        "name": "Z1",
        "position": [7.0, 8.0, 9.0],
        "vel_limits": [8.5, 8.5, 8.5],
        "inverted": false
      },
      {
        "id": 4,
        "name": "W1",
        "position": [10.0, 11.0, 12.0],
        "vel_limits": [7.5, 7.5, 7.5],
        "inverted": true
      }
    ],
    "sensors": [
      {
        "type": "imu",
        "model": "IMU-9000",
        "range_min": -3.14,
        "range_max": 3.14,
        "active": true
      },
      {
        "type": "lidar",
        "model": "LIDAR-X20",
        "range_min": 0.2,
        "range_max": 30.0,
        "active": true
      },
      {
        "type": "encoder",
        "model": "ENC-5000",
        "range_min": 0.0,
        "range_max": 1000.0,
        "active": false
      },
      {
        "type": "temp",
        "model": "TMP-100",
        "range_min": -40.0,
        "range_max": 125.0,
        "active": true
      }
    ]
  },

  "logging": {
    "enabled": true,
    "path": "/var/log/static_bench",
    "max_files": 8
  }
}
)JSON";

bool ParseWithJSONReflection2_Static(static_model::StaticComplexConfig & cfg) {
    auto res = JSONReflection2::Parse(cfg, kJsonStatic);
    if (!res) {
        // If you need debugging:
        // auto err = res.error();
        // auto pos = res.pos();
        return false;
    }
    return true;
}

bool ParseWithJSONReflection2_Dynamic(dynamic_model::DynamicComplexConfig& cfg) {
    auto res = JSONReflection2::Parse(cfg, kJsonStatic);
    if (!res) {
        // If you need debugging:
        // auto err = res.error();
        // auto pos = res.pos();
        return false;
    }
    return true;
}

// ----------------------
// Main benchmark
// ----------------------


bool ParseWithRapidJSON() {
    rj::Document doc;
    doc.Parse(kJsonStatic.data(), kJsonStatic.size());
    if (doc.HasParseError()) {
        std::cerr << "RapidJSON parse error: "
                  << rj::GetParseError_En(doc.GetParseError())
                  << " at offset " << doc.GetErrorOffset() << "\n";
        return false;
    }
    return true;
}



int main() {
    constexpr int iterations = 10000000;

    std::ios::sync_with_stdio(false);



    using clock = std::chrono::steady_clock;


    std::cout << "iterations: " << iterations << std::endl;
    {
        static_model::StaticComplexConfig cfg{};
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i) {
            if (!ParseWithJSONReflection2_Static(cfg)) {
                std::cerr << "JSONReflection2 static parse failed\n";
                return 1;
            }
        }
        std::string_view(cfg.app_name.data()) == "fuu";
        
        auto end   = std::chrono::steady_clock::now();
        auto total = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << "JSONReflection2 (static containers): total "
                  << total << " us, avg " << double(total) / iterations << " us/parse\n";
    }

     {
        dynamic_model::DynamicComplexConfig cfg{};
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i) {
            if (!ParseWithJSONReflection2_Dynamic(cfg)) {
                std::cerr << "JSONReflection2 dynamic parse failed\n";
                return 1;
            }
        }
        cfg.app_name == "fuuu";
        auto end   = std::chrono::steady_clock::now();
        auto total = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << "JSONReflection2 (dynamic containers): total "
                  << total << " us, avg " << double(total) / iterations << " us/parse\n";
    }

    {
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i) {
            if (!ParseWithRapidJSON()) {
                std::cerr << "RapidJSON  parse failed\n";
                return 1;
            }
        }
        auto end   = std::chrono::steady_clock::now();
        auto total = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << "RapidJSON parsing only, without mapping: total "
                  << total << " us, avg " << double(total) / iterations << " us/parse\n";
    }

  



    return 0;
}
