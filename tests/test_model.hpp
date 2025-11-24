#include <array>
#include <cstdint>
#include <list>
#include <optional>
#include <string>
#include <vector>


#include <JsonFusion/parser.hpp>

namespace json_fusion_test_models {

namespace static_model {

using namespace JsonFusion::options;
using JsonFusion::Annotated;

using SmallStr  = std::array<char, 16>;
using MediumStr = std::array<char, 32>;
using LargeStr  = std::array<char, 64>;

// ---------- Sub-structures ----------

// Network interface configuration
struct Network {
    SmallStr   name;
    LargeStr   address; // e.g. "192.168.0.1/24"
    int        port;
    bool       enabled;
};

// Motor channel configuration
struct Motor {
    int64_t         id;
    SmallStr        name;
    Annotated<std::array<
        Annotated<double, range<-1000, 1000>>,3>, min_items<3>>      position;        // [x,y,z]
    Annotated<std::array<
        Annotated<float,  range<-1000, 1000>>,3>, min_items<3>>    vel_limits;      // [vx,vy,vz]
    bool           inverted;
};

// Sensor configuration
struct Sensor {
    SmallStr    type;    
    MediumStr   model;
    Annotated<float, range<-100, 100000> >  range_min;
    double      range_max;
    bool        active;
};

// Controller-level configuration
struct Controller {
    MediumStr                             name;
    Annotated<int, range<200, 10000>>     loop_hz;
    Annotated<std::array<Motor, 4>, min_items<1>>    motors;
    std::array<Sensor, 4>  sensors;
};

// Logging configuration
struct Logging {
    bool        enabled;
    LargeStr    path;
    uint32_t    max_files;
};

// Top-level config
struct ComplexConfig {
    MediumStr         app_name;
    uint16_t          version_major;
    int               version_minor;
    Network           network;
    std::optional<Network> fallback_network_conf;
    Controller        controller;
    Logging           logging;
};
}

namespace dynamic_model {

using std::array;
using std::int64_t;
using std::list;
using std::optional;
using std::string;
using std::string_view;
using std::vector;

using namespace JsonFusion::options;
using JsonFusion::Annotated;

struct Network {
    string    name;
    string  address; // e.g. "192.168.0.1/24"
    int     port;
    bool    enabled;
};

// Motor channel configuration
struct Motor {
    int64_t             id;
    string              name;
    Annotated<vector<
            Annotated<double, range<-1000, 1000>>
        >,  max_items<3>, min_items<3>>      position;  
    Annotated<vector<
            Annotated<float, range<-1000, 1000>>
        >,  max_items<3>, min_items<3>>      vel_limits; 
    bool                                     inverted;
};

// Sensor configuration
struct Sensor {
    string    type;       // "lidar", "imu", ...
    string    model;
    Annotated<float, range<-100, 100000> >  range_min;
    double    range_max;
    bool      active;
};

// Controller-level configuration
struct Controller {
    string                                                  name;
    Annotated<int, range<200, 10000>>                       loop_hz;
    Annotated<list<Motor>,  min_items<1>,  max_items<4>>    motors;
    Annotated<list<Sensor>, max_items<4>>                   sensors;
};

// Logging configuration
struct Logging {
    bool      enabled;
    string    path;
    uint32_t  max_files;
};

// Top-level config
struct ComplexConfig {
    string                 app_name;
    uint16_t               version_major;
    int                    version_minor;
    Network                network;
    optional<Network>      fallback_network_conf;
    Controller             controller;
    Logging                logging;
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
      "fallback_network_conf": null,
      "logging": {
        "enabled": true,
        "path": "/var/log/static_bench",
        "max_files": 8
      }
      
    }
    )JSON";
}
