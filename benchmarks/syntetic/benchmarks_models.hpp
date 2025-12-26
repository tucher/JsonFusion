#pragma once

#include <array>
#include <optional>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>

#include "JsonFusion/annotated.hpp"
#include "JsonFusion/validators.hpp"




namespace json_fusion_benchmarks {
using namespace JsonFusion::validators;
using namespace JsonFusion::options;

template<class T, class ... O>
using A = JsonFusion::Annotated<T, O...>;

using std::array, std::optional, std::list, std::vector, std::string, std::map, std::unordered_map;



struct EmbeddedConfigSmall {
    static constexpr std::string_view name = "EmbeddedConfig/small";


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

    static constexpr std::size_t kMult = 16;
    using SmallStr  = array<char, 16 * kMult>;
    using MediumStr = array<char, 32 * kMult>;
    using LargeStr  = array<char, 64 * kMult>;



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

    static constexpr std::size_t iter_count = 100000;

    // choose one default "json" – we can add more fields if needed

};

struct TelemetrySample {
    static constexpr std::string_view name = "TelemetrySample";
    static constexpr std::size_t iter_count = 100000;

    static constexpr std::string_view json = R"JSON(
        {
            "samples": [
                {
                  "device_id": "dev-123",
                  "timestamp": 1710000000,
                  "battery": 3.71,
                  "temp_c": 22.5,
                  "errors": [],
                  "tags": { "region": "eu-west-1", "fw": "1.0.3" },
                  "accel": [0.01, 0.02, 0.03]
                },
                {
                  "device_id": "dev-124",
                  "timestamp": 1710000005,
                  "battery": 3.69,
                  "temp_c": 22.4,
                  "errors": ["low_batt"],
                  "tags": { "region": "eu-west-1", "fw": "1.0.3" },
                  "accel": [0.00, 0.01, 0.02]
                },
                {
                  "device_id": "dev-123",
                  "timestamp": 1710000000,
                  "battery": 3.71,
                  "temp_c": 22.5,
                  "errors": [],
                  "tags": { "region": "eu-west-1", "fw": "1.0.3" },
                  "accel": [0.01, 0.02, 0.03]
                },
                {
                  "device_id": "dev-124",
                  "timestamp": 1710000005,
                  "battery": 3.69,
                  "temp_c": 22.4,
                  "errors": ["low_batt"],
                  "tags": { "region": "eu-west-1", "fw": "1.0.3" },
                  "accel": [0.00, 0.01, 0.02]
                },
                {
                  "device_id": "dev-123",
                  "timestamp": 1710000000,
                  "battery": 3.71,
                  "temp_c": 22.5,
                  "errors": [],
                  "tags": { "region": "eu-west-1", "fw": "1.0.3" },
                  "accel": [0.01, 0.02, 0.03]
                },
                {
                  "device_id": "dev-124",
                  "timestamp": 1710000005,
                  "battery": 3.69,
                  "temp_c": 22.4,
                  "errors": ["low_batt"],
                  "tags": { "region": "eu-west-1", "fw": "1.0.3" },
                  "accel": [0.00, 0.01, 0.02]
                },
                {
                  "device_id": "dev-123",
                  "timestamp": 1710000000,
                  "battery": 3.71,
                  "temp_c": 22.5,
                  "errors": [],
                  "tags": { "region": "eu-west-1", "fw": "1.0.3" },
                  "accel": [0.01, 0.02, 0.03]
                },
                {
                  "device_id": "dev-124",
                  "timestamp": 1710000005,
                  "battery": 3.69,
                  "temp_c": 22.4,
                  "errors": ["low_batt"],
                  "tags": { "region": "eu-west-1", "fw": "1.0.3" },
                  "accel": [0.00, 0.01, 0.02]
                }
            ]
        }
    )JSON";


    static constexpr std::size_t kMaxErrors = 8;
    static constexpr std::size_t kMaxTags   = 8;

    static constexpr std::size_t kMult = 16;
    using SmallStr  = array<char, 16 * kMult>;
    using MediumStr = array<char, 32 * kMult>;
    using LargeStr  = array<char, 64 * kMult>;


    struct TelemetrySampleStatic {
        SmallStr device_id;  // e.g. "dev-123"
        int64_t  timestamp;  // unix seconds

        A<float, range<0, 5>>   battery; // volts
        A<float, range<-100, 150>> temp_c;

        A<array<SmallStr, kMaxErrors>, max_items<kMaxErrors>> errors;

        // fixed set of tags for embedded; dynamic variant will use map
        struct TagKV {
            SmallStr key;
            SmallStr value;
        };
        A<array<TagKV, kMaxTags>, max_items<kMaxTags>> tags;

        // optional accelerometer sample as array [x, y, z]
        struct Vec3 {
            float x;
            float y;
            float z;
        };
        A<optional<Vec3>, as_array> accel;
    };


    struct TelemetrySampleDynamic {
        A<string, min_length<1>> device_id;
        int64_t  timestamp;

        A<float, range<0, 5>>     battery;
        A<float, range<-100, 150>> temp_c;

        vector<string> errors;

        A<
            unordered_map<string, string>,
            max_properties<16>, max_key_length<32>
            > tags;

        struct Vec3 {
            float x;
            float y;
            float z;
        };
        A<optional<Vec3>, as_array> accel;
    };
    struct SamplesDynamic{
        list<TelemetrySampleDynamic> samples;

    };

    // using StaticModel  = TelemetrySampleStatic;
    using DynamicModel = SamplesDynamic;



};


struct RPCCommand {
    static constexpr std::string_view name = "RPC Command";

    static constexpr std::size_t iter_count = 100000;

    static constexpr std::string_view json = R"JSON(
        {
            "commands": [
                {
                  "cmd": "set_param",
                  "set_param": {
                    "loop_hz": 1000,
                    "logging_enabled": true,
                    "log_level": "info"
                  }
                },
                {
                  "cmd": "start_job",
                  "id": "corr-456",
                  "start_job": {
                    "job_id": "job-42",
                    "mode": "normal"
                  }
                },
                {
                  "cmd": "set_param",
                  "set_param": {
                    "loop_hz": null,
                    "logging_enabled": true,
                    "log_level": "info"
                  }
                },
                {
                  "cmd": "start_job",
                  "id": "corr-456",
                  "start_job": {
                    "job_id": "job-42",
                    "mode": "normal"
                  }
                },
                {
                  "cmd": "set_param",
                  "set_param": {
                    "loop_hz": null,
                    "logging_enabled": true,
                    "log_level": "info"
                  }
                },
                {
                  "cmd": "start_job",
                  "id": "corr-456",
                  "start_job": {
                    "job_id": "job-42",
                    "mode": "normal"
                  }
                },
                {
                  "cmd": "set_param",
                  "set_param": {
                    "loop_hz": null,
                    "logging_enabled": true,
                    "log_level": "info"
                  }
                },
                {
                  "cmd": "start_job",
                  "id": "corr-456",
                  "start_job": {
                    "job_id": "job-42",
                    "mode": "normal"
                  }
                },
                {
                  "cmd": "set_param",
                  "set_param": {
                    "loop_hz": null,
                    "logging_enabled": true,
                    "log_level": "info"
                  }
                }
            ]
        }
    )JSON";

    struct SetParamPayload {
        optional<int>        loop_hz;
        optional<bool>       logging_enabled;
        optional<string> log_level;
    };

    struct StartJobPayload {
        A<string, min_length<1>> job_id;
        optional<string>           mode;
    };

    // Simple tagged union via "cmd" + payload object
    struct Command {
        A<string, enum_values<"set_param", "start_job", "stop_job"> > cmd;

        A<std::string, key<"id">> correlation_id;

        optional<SetParamPayload> set_param;
        optional<StartJobPayload> start_job;

        // purely internal, not in JSON
        A<std::string, not_json> debug_source;
    };

    struct TopLevel {
        std::vector<Command> commands;
    };

    using DynamicModel = TopLevel;

};

struct LogEvent {
    static constexpr std::string_view name = "Log events";

    static constexpr std::size_t iter_count = 100000;

    static constexpr std::string_view json = R"JSON(
    [
        {
          "ts": "2025-01-01T12:34:56.789Z",
          "level": "INFO",
          "logger": "order-service",
          "message": "Order created",
          "order_id": "ord-123",
          "user_id": "usr-999",
          "context": {
            "ip": "192.168.1.5",
            "session": "abc123",
            "retry": "1"
          },
          "tags": {
            "region": "eu-west",
            "node": "node-7"
          }
        },
        {
          "ts": "2025-01-01T12:34:56.789Z",
          "level": "INFO",
          "logger": "order-service",
          "message": "Order created",
          "order_id": "ord-123",
          "user_id": "usr-999",
          "context": {
            "ip": "192.168.1.5",
            "session": "abc123",
            "retry": "1"
          },
          "tags": {
            "region": "eu-west",
            "node": "node-7"
          }
        },
        {
          "ts": "2025-01-01T12:34:56.789Z",
          "level": "INFO",
          "logger": "order-service",
          "message": "Order created",
          "order_id": "ord-123",
          "user_id": "usr-999",
          "context": {
            "ip": "192.168.1.5",
            "session": "abc123",
            "retry": "1"
          },
          "tags": {
            "region": "eu-west",
            "node": "node-7"
          }
        },
        {
          "ts": "2025-01-01T12:34:56.789Z",
          "level": "INFO",
          "logger": "order-service",
          "message": "Order created",
          "order_id": "ord-123",
          "user_id": "usr-999",
          "context": {
            "ip": "192.168.1.5",
            "session": "abc123",
            "retry": "1"
          },
          "tags": {
            "region": "eu-west",
            "node": "node-7"
          }
        }
    ]
    )JSON";

    struct Entry {
        // JSON key: "ts" → C++ field: timestamp
        A<string, key<"ts">> timestamp;

        A<string,
          enum_values<"TRACE", "DEBUG", "INFO", "WARN", "ERROR">
          > level;

        string logger;
        string message;

        optional<string> order_id;
        optional<string> user_id;

        A<
            unordered_map<string, string>,
            max_properties<32>, max_key_length<32>
            > context;

        A<
            unordered_map<string, string>,
            max_properties<32>, max_key_length<32>
            > tags;
    };


    using DynamicModel = std::vector<Entry>;
};


struct BusEvents_MessagePayloads {
    static constexpr std::string_view name = "Bus Events / Message Payloads";

    static constexpr std::size_t iter_count = 100000;

    static constexpr std::string_view json = R"JSON(
     [
        {
          "event_type": "OrderCreated",
          "event_version": 3,
          "event_id": "ev-123",
          "timestamp": 1710000000,
          "payload": {
            "order_id": "ord-123",
            "customer_id": "cus-321",
            "currency": "USD",
            "lines": [
              { "sku": "A", "qty": 2, "price": 9.99 },
              { "sku": "B", "qty": 1, "price": 5.0 }
            ],
            "total": 24.98
          },
          "meta": {
            "source": "checkout",
            "trace_id": "trace-abc",
            "shard": "5"
          }
        },
        {
          "event_type": "OrderPaid",
          "event_version": 1,
          "event_id": "ev-124",
          "timestamp": 1710000010,
          "payload": {
            "order_id": "ord-123",
            "customer_id": "cus-321",
            "currency": "USD",
            "lines": [
              { "sku": "A", "qty": 2, "price": 9.99 },
              { "sku": "B", "qty": 1, "price": 5.0 }
            ],
            "total": 24.98
          },
          "meta": {
            "source": "payments",
            "trace_id": "trace-abc",
            "shard": "5"
          }
        },
        {
          "event_type": "OrderPaid",
          "event_version": 1,
          "event_id": "ev-124",
          "timestamp": 1710000010,
          "payload": {
            "order_id": "ord-123",
            "customer_id": "cus-321",
            "currency": "USD",
            "lines": [
              { "sku": "A", "qty": 2, "price": 9.99 },
              { "sku": "B", "qty": 1, "price": 5.0 }
            ],
            "total": 24.98
          },
          "meta": {
            "source": "payments",
            "trace_id": "trace-abc",
            "shard": "5"
          }
        },
        {
          "event_type": "OrderPaid",
          "event_version": 1,
          "event_id": "ev-124",
          "timestamp": 1710000010,
          "payload": {
            "order_id": "ord-123",
            "customer_id": "cus-321",
            "currency": "USD",
            "lines": [
              { "sku": "A", "qty": 2, "price": 9.99 },
              { "sku": "B", "qty": 1, "price": 5.0 }
            ],
            "total": 24.98
          },
          "meta": {
            "source": "payments",
            "trace_id": "trace-abc",
            "shard": "5"
          }
        },
        {
          "event_type": "OrderPaid",
          "event_version": 1,
          "event_id": "ev-124",
          "timestamp": 1710000010,
          "payload": {
            "order_id": "ord-123",
            "customer_id": "cus-321",
            "currency": "USD",
            "lines": [
              { "sku": "A", "qty": 2, "price": 9.99 },
              { "sku": "B", "qty": 1, "price": 5.0 }
            ],
            "total": 24.98
          },
          "meta": {
            "source": "payments",
            "trace_id": "trace-abc",
            "shard": "5"
          }
        }
      ]

    )JSON";

    struct Event {
        A<
            string,
            enum_values<"OrderCreated", "OrderPaid", "OrderCancelled">
            > event_type;

        int         event_version;
        string event_id;
        int64_t     timestamp;


        struct OrderPayload {
            A<string, min_length<1>> order_id;
            A<string, min_length<1>> customer_id;
            A<string, enum_values<"USD", "EUR", "GBP">> currency;

            struct OrderLine {
                A<string, min_length<1>> sku;
                A<int, range<1, 1000000>>     qty;
                A<double, range<0, 1'000'000>> price;
            };

            A<vector<OrderLine>, min_items<1>> lines;

            A<double, range<0, 1'000'000>> total;
        };

        OrderPayload payload;

        using EventMeta = A<  unordered_map<string, string>,
                            max_properties<32>, max_key_length<64>
                            >;

        EventMeta    meta;
    };
    using DynamicModel = std::vector<Event>;


};

struct MetricsTimeSeries {
    static constexpr std::string_view name = "Metrics / Time-Series Samples";

    static constexpr std::size_t iter_count = 100000;

    static constexpr std::string_view json = R"JSON(
[
    {
      "metric": "http_requests_total",
      "value": 1234,
      "ts": 1710000000,
      "labels": {
        "service": "auth",
        "method": "GET",
        "code": "200"
      }
    },
    {
      "metric": "http_requests_total",
      "value": 12,
      "ts": 1710000001,
      "labels": {
        "service": "auth",
        "method": "POST",
        "code": "500"
      }
    },
    {
      "metric": "http_requests_total",
      "value": 1234,
      "ts": 1710000000,
      "labels": {
        "service": "auth",
        "method": "GET",
        "code": "200"
      }
    },
    {
      "metric": "http_requests_total",
      "value": 1234,
      "ts": 1710000000,
      "labels": {
        "service": "auth",
        "method": "GET",
        "code": "200"
      }
    },
    {
      "metric": "http_requests_total",
      "value": 12,
      "ts": 1710000001,
      "labels": {
        "service": "auth",
        "method": "POST",
        "code": "500"
      }
    },
    {
      "metric": "http_requests_total",
      "value": 1234,
      "ts": 1710000000,
      "labels": {
        "service": "auth",
        "method": "GET",
        "code": "200"
      }
    },
    {
      "metric": "http_requests_total",
      "value": 1234,
      "ts": 1710000000,
      "labels": {
        "service": "auth",
        "method": "GET",
        "code": "200"
      }
    },
    {
      "metric": "http_requests_total",
      "value": 12,
      "ts": 1710000001,
      "labels": {
        "service": "auth",
        "method": "POST",
        "code": "500"
      }
    },
    {
      "metric": "http_requests_total",
      "value": 1234,
      "ts": 1710000000,
      "labels": {
        "service": "auth",
        "method": "GET",
        "code": "200"
      }
    }
  ]
    )JSON";

    struct MetricSample {
        A<string, min_length<1>> metric;
        double      value;
        int64_t     ts;

        A<
            unordered_map<string, string>,
            max_properties<16>, max_key_length<32>
            > labels;
    };
    using DynamicModel = std::vector<MetricSample>;
};



}

