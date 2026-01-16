#pragma once

#include <array>
#include <optional>
#include <cstdint>

// Additional models for code size comparison between glaze and JsonFusion
// All models use fixed-size buffers and optionals, no validators
// Designed to be usable by both libraries without modifications

namespace code_size_test {

using TinyStr   = std::array<char, 8>;
using SmallStr  = std::array<char, 16>;
using MediumStr = std::array<char, 32>;
using LargeStr  = std::array<char, 64>;
using HugeStr   = std::array<char, 128>;

// Model 1: DeviceMetadata - Device information with nested capabilities
struct DeviceMetadata {
    static constexpr std::size_t kMaxCapabilities = 8;
    static constexpr std::size_t kMaxInterfaces = 4;
    
    MediumStr device_id;
    SmallStr  manufacturer;
    SmallStr  model;
    uint32_t  firmware_version;
    uint64_t  serial_number;
    
    struct Hardware {
        SmallStr  cpu_model;
        uint32_t  cpu_freq_mhz;
        uint32_t  ram_bytes;
        uint32_t  flash_bytes;
        
        struct Peripheral {
            TinyStr  name;
            uint16_t address;
            bool     enabled;
        };
        std::array<Peripheral, 8> peripherals;
        size_t peripherals_count;
    };
    Hardware hardware;
    
    struct NetworkInterface {
        TinyStr   type;  // "eth", "wifi", "ble"
        MediumStr mac_address;
        MediumStr ip_address;
        uint16_t  mtu;
        std::optional<uint32_t> speed_mbps;
    };
    std::array<NetworkInterface, kMaxInterfaces> interfaces;
    size_t interfaces_count;
    
    struct Capability {
        SmallStr name;
        SmallStr version;
        bool     supported;
    };
    std::array<Capability, kMaxCapabilities> capabilities;
    size_t capabilities_count;
};

// Model 2: SensorReadings - Time-series sensor data
struct SensorReadings {
    static constexpr std::size_t kMaxSamples = 16;
    static constexpr std::size_t kMaxChannels = 4;
    
    uint64_t  timestamp_us;
    SmallStr  sensor_id;
    TinyStr   unit;
    
    struct Sample {
        uint32_t time_offset_us;
        double   value;
        uint8_t  quality;
        std::optional<double> error_margin;
        
        struct Metadata {
            uint16_t raw_adc;
            float    temperature_c;
            uint8_t  gain;
        };
        std::optional<Metadata> metadata;
    };
    
    struct Channel {
        TinyStr  name;
        std::array<Sample, kMaxSamples> samples;
        size_t samples_count;
        
        struct Calibration {
            double offset;
            double scale;
            uint32_t last_calibration_timestamp;
        };
        std::optional<Calibration> calibration;
    };
    std::array<Channel, kMaxChannels> channels;
    size_t channels_count;
};

// Model 3: SystemStats - Performance and resource metrics
struct SystemStats {
    static constexpr std::size_t kMaxTasks = 12;
    static constexpr std::size_t kMaxMemRegions = 6;
    
    uint64_t uptime_seconds;
    uint32_t boot_count;
    
    struct CPU {
        float    utilization_percent;
        uint32_t frequency_mhz;
        float    temperature_c;
        uint32_t context_switches;
        uint32_t interrupts;
        
        struct Core {
            uint8_t id;
            float   load_percent;
            uint32_t idle_time_ms;
        };
        std::array<Core, 4> cores;
        size_t cores_count;
    };
    CPU cpu;
    
    struct Memory {
        uint32_t total_bytes;
        uint32_t used_bytes;
        uint32_t peak_bytes;
        uint32_t allocations;
        uint32_t deallocations;
        
        struct Region {
            SmallStr name;
            uint32_t base_address;
            uint32_t size_bytes;
            uint32_t used_bytes;
        };
        std::array<Region, kMaxMemRegions> regions;
        size_t regions_count;
    };
    Memory memory;
    
    struct Task {
        SmallStr name;
        uint16_t priority;
        uint32_t stack_size;
        uint32_t stack_used;
        float    cpu_percent;
        std::optional<uint32_t> wakeup_count;
    };
    std::array<Task, kMaxTasks> tasks;
    size_t tasks_count;
};

// Model 4: NetworkPacket - Network protocol message
struct NetworkPacket {
    static constexpr std::size_t kMaxHeaders = 8;
    static constexpr std::size_t kMaxPayloadChunks = 4;
    
    uint32_t packet_id;
    uint16_t sequence;
    TinyStr  protocol;
    
    struct Address {
        MediumStr host;
        uint16_t  port;
        TinyStr   family;  // "ipv4", "ipv6"
    };
    Address source;
    Address destination;
    
    struct Header {
        SmallStr key;
        MediumStr value;
    };
    std::array<Header, kMaxHeaders> headers;
    size_t headers_count;
    
    struct Payload {
        TinyStr encoding;
        uint32_t total_size;
        uint32_t checksum;
        
        struct Chunk {
            uint16_t offset;
            uint16_t length;
            std::array<uint8_t, 32> data;
        };
        std::array<Chunk, kMaxPayloadChunks> chunks;
        size_t chunks_count;
    };
    Payload payload;
    
    struct Routing {
        uint8_t  ttl;
        uint8_t  hops;
        MediumStr next_hop;
        std::optional<MediumStr> return_path;
    };
    std::optional<Routing> routing;
};

// Model 5: ImageDescriptor - Image metadata with histogram
struct ImageDescriptor {
    static constexpr std::size_t kMaxHistogramBins = 16;
    static constexpr std::size_t kMaxTags = 8;
    
    MediumStr filename;
    uint32_t  width;
    uint32_t  height;
    TinyStr   format;  // "jpg", "png", "bmp"
    uint32_t  size_bytes;
    
    struct ColorSpace {
        TinyStr  model;  // "rgb", "hsv", "yuv"
        uint8_t  bits_per_channel;
        uint8_t  channels;
        
        struct Histogram {
            TinyStr channel;
            std::array<uint32_t, kMaxHistogramBins> bins;
            double mean;
            double stddev;
        };
        std::array<Histogram, 4> histograms;
        size_t histograms_count;
    };
    ColorSpace color_space;
    
    struct EXIF {
        uint64_t  timestamp;
        MediumStr camera_model;
        float     focal_length_mm;
        float     aperture;
        uint32_t  iso;
        float     exposure_time_s;
        
        struct GPS {
            double latitude;
            double longitude;
            float  altitude_m;
        };
        std::optional<GPS> gps;
    };
    std::optional<EXIF> exif;
    
    struct Tag {
        SmallStr key;
        SmallStr value;
    };
    std::array<Tag, kMaxTags> tags;
    size_t tags_count;
};

// Model 6: AudioConfig - Audio processing configuration
struct AudioConfig {
    static constexpr std::size_t kMaxFilters = 8;
    static constexpr std::size_t kMaxEffects = 6;
    
    MediumStr preset_name;
    uint32_t  sample_rate;
    uint8_t   bit_depth;
    uint8_t   channels;
    
    struct Input {
        TinyStr  source;  // "mic", "line", "usb"
        float    gain_db;
        bool     phantom_power;
        uint16_t buffer_size;
        
        struct Compressor {
            float threshold_db;
            float ratio;
            float attack_ms;
            float release_ms;
            float knee_db;
        };
        std::optional<Compressor> compressor;
    };
    Input input;
    
    struct Filter {
        TinyStr  type;  // "lpf", "hpf", "bpf", "notch"
        float    frequency_hz;
        float    q_factor;
        float    gain_db;
        bool     enabled;
    };
    std::array<Filter, kMaxFilters> filters;
    size_t filters_count;
    
    struct Effect {
        SmallStr name;
        float    mix_percent;
        bool     enabled;
        
        struct Parameter {
            TinyStr name;
            float   value;
            float   min_value;
            float   max_value;
        };
        std::array<Parameter, 4> parameters;
        size_t parameters_count;
    };
    std::array<Effect, kMaxEffects> effects;
    size_t effects_count;
    
    struct Output {
        TinyStr  destination;
        float    volume_db;
        bool     mute;
        float    limiter_threshold_db;
    };
    Output output;
};

// Model 7: CacheEntry - Cache entry with metadata
struct CacheEntry {
    static constexpr std::size_t kMaxDependencies = 8;
    static constexpr std::size_t kMaxDataBlocks = 4;
    
    LargeStr  key;
    uint64_t  hash;
    uint32_t  size_bytes;
    uint64_t  created_timestamp;
    uint64_t  accessed_timestamp;
    uint32_t  access_count;
    
    struct Priority {
        uint8_t  level;
        float    score;
        bool     pinned;
        std::optional<uint64_t> expiry_timestamp;
    };
    Priority priority;
    
    struct Metadata {
        MediumStr content_type;
        TinyStr   encoding;
        uint32_t  original_size;
        uint32_t  compressed_size;
        uint32_t  checksum;
        
        struct Compression {
            TinyStr  algorithm;
            uint8_t  level;
            float    ratio;
        };
        std::optional<Compression> compression;
    };
    Metadata metadata;
    
    struct DataBlock {
        uint16_t offset;
        uint16_t length;
        uint32_t crc32;
        std::array<uint8_t, 64> data;
    };
    std::array<DataBlock, kMaxDataBlocks> data_blocks;
    size_t data_blocks_count;
    
    struct Dependency {
        LargeStr key;
        uint64_t hash;
        bool     required;
    };
    std::array<Dependency, kMaxDependencies> dependencies;
    size_t dependencies_count;
};

// Model 8: FileMetadata - Filesystem metadata
struct FileMetadata {
    static constexpr std::size_t kMaxExtendedAttrs = 8;
    static constexpr std::size_t kMaxChunks = 12;
    
    LargeStr  path;
    MediumStr name;
    TinyStr   extension;
    uint64_t  size_bytes;
    uint64_t  inode;
    
    struct Timestamps {
        uint64_t created;
        uint64_t modified;
        uint64_t accessed;
        std::optional<uint64_t> deleted;
    };
    Timestamps timestamps;
    
    struct Permissions {
        uint16_t mode;
        uint32_t owner_uid;
        uint32_t group_gid;
        bool     readable;
        bool     writable;
        bool     executable;
    };
    Permissions permissions;
    
    struct Storage {
        uint32_t block_size;
        uint32_t blocks_allocated;
        TinyStr  filesystem;
        SmallStr device;
        
        struct Chunk {
            uint64_t offset;
            uint32_t length;
            uint64_t physical_block;
            bool     compressed;
        };
        std::array<Chunk, kMaxChunks> chunks;
        size_t chunks_count;
    };
    Storage storage;
    
    struct Checksum {
        TinyStr  algorithm;
        HugeStr  value;
        uint64_t computed_timestamp;
    };
    std::optional<Checksum> checksum;
    
    struct ExtendedAttr {
        SmallStr name;
        MediumStr value;
    };
    std::array<ExtendedAttr, kMaxExtendedAttrs> extended_attrs;
    size_t extended_attrs_count;
};

// Model 9: TransactionRecord - Financial transaction
struct TransactionRecord {
    static constexpr std::size_t kMaxLineItems = 10;
    static constexpr std::size_t kMaxTags = 6;
    
    LargeStr  transaction_id;
    uint64_t  timestamp_ms;
    TinyStr   currency;
    TinyStr   status;  // "pending", "completed", "failed"
    
    struct Party {
        MediumStr id;
        MediumStr name;
        TinyStr   type;  // "user", "merchant", "bank"
        
        struct Account {
            MediumStr number;
            SmallStr  routing;
            TinyStr   type;
            uint64_t  balance_cents;
        };
        Account account;
        
        struct Address {
            MediumStr street;
            SmallStr  city;
            TinyStr   state;
            SmallStr  postal_code;
            TinyStr   country;
        };
        std::optional<Address> address;
    };
    Party sender;
    Party receiver;
    
    struct LineItem {
        SmallStr  description;
        uint64_t  amount_cents;
        float     tax_rate;
        uint64_t  tax_cents;
        uint32_t  quantity;
        
        struct Details {
            SmallStr sku;
            TinyStr  category;
            std::optional<float> discount_percent;
        };
        std::optional<Details> details;
    };
    std::array<LineItem, kMaxLineItems> line_items;
    size_t line_items_count;
    
    struct Totals {
        uint64_t subtotal_cents;
        uint64_t tax_cents;
        uint64_t fees_cents;
        uint64_t total_cents;
    };
    Totals totals;
    
    struct Security {
        SmallStr signature;
        TinyStr  algorithm;
        uint64_t nonce;
    };
    std::optional<Security> security;
    
    struct Tag {
        TinyStr key;
        SmallStr value;
    };
    std::array<Tag, kMaxTags> tags;
    size_t tags_count;
};

// Model 10: TelemetryPacket - Telemetry data stream
struct TelemetryPacket {
    static constexpr std::size_t kMaxMetrics = 16;
    static constexpr std::size_t kMaxEvents = 8;
    
    uint64_t  packet_id;
    uint64_t  timestamp_ns;
    SmallStr  source_id;
    uint16_t  sequence;
    
    struct Source {
        MediumStr device_id;
        MediumStr component;
        SmallStr  version;
        
        struct Location {
            float latitude;
            float longitude;
            float altitude_m;
            float heading_deg;
            float speed_mps;
        };
        std::optional<Location> location;
    };
    Source source;
    
    struct Metric {
        SmallStr name;
        TinyStr  unit;
        double   value;
        double   min_value;
        double   max_value;
        uint32_t sample_count;
        
        struct Statistics {
            double mean;
            double stddev;
            double median;
            std::optional<double> percentile_95;
        };
        std::optional<Statistics> statistics;
    };
    std::array<Metric, kMaxMetrics> metrics;
    size_t metrics_count;
    
    struct Event {
        uint64_t timestamp_offset_ns;
        TinyStr  severity;  // "info", "warn", "error"
        SmallStr type;
        MediumStr message;
        
        struct Context {
            SmallStr function;
            uint32_t line;
            TinyStr  thread;
        };
        std::optional<Context> context;
    };
    std::array<Event, kMaxEvents> events;
    size_t events_count;
    
    struct Health {
        uint8_t  score;
        bool     degraded;
        uint32_t error_count;
        std::optional<MediumStr> last_error;
    };
    Health health;
};

// Model 11: RobotCommand - Robotics control command
struct RobotCommand {
    static constexpr std::size_t kMaxJoints = 8;
    static constexpr std::size_t kMaxWaypoints = 6;
    
    uint64_t  command_id;
    uint64_t  timestamp_us;
    SmallStr  robot_id;
    TinyStr   mode;  // "auto", "manual", "emergency"
    
    struct Kinematics {
        struct Joint {
            TinyStr  name;
            float    angle_deg;
            float    velocity_deg_s;
            float    torque_nm;
            float    temperature_c;
            bool     homed;
        };
        std::array<Joint, kMaxJoints> joints;
        size_t joints_count;
        
        struct EndEffector {
            std::array<float, 3> position;
            std::array<float, 4> quaternion;
            float    gripper_state;
            bool     tool_engaged;
        };
        EndEffector end_effector;
    };
    Kinematics kinematics;
    
    struct Trajectory {
        TinyStr  interpolation;  // "linear", "cubic", "quintic"
        float    duration_s;
        
        struct Waypoint {
            float    time_offset_s;
            std::array<float, 6> joint_positions;
            std::optional<std::array<float, 6>> joint_velocities;
        };
        std::array<Waypoint, kMaxWaypoints> waypoints;
        size_t waypoints_count;
    };
    std::optional<Trajectory> trajectory;
    
    struct Safety {
        float    max_velocity_limit;
        float    max_acceleration_limit;
        bool     collision_detection_enabled;
        std::array<float, 6> workspace_bounds_min;
        std::array<float, 6> workspace_bounds_max;
    };
    Safety safety;
};

// Model 12: WeatherData - Meteorological data
struct WeatherData {
    static constexpr std::size_t kMaxHourlyForecasts = 12;
    static constexpr std::size_t kMaxAlerts = 4;
    
    uint64_t  timestamp;
    MediumStr station_id;
    
    struct Location {
        float    latitude;
        float    longitude;
        float    elevation_m;
        MediumStr name;
        TinyStr  timezone;
    };
    Location location;
    
    struct CurrentConditions {
        float    temperature_c;
        float    feels_like_c;
        float    humidity_percent;
        float    pressure_hpa;
        float    wind_speed_mps;
        float    wind_direction_deg;
        float    visibility_m;
        float    uv_index;
        
        struct Precipitation {
            float    rain_1h_mm;
            float    rain_24h_mm;
            float    snow_1h_mm;
            float    snow_24h_mm;
        };
        Precipitation precipitation;
        
        struct CloudCover {
            uint8_t  total_percent;
            uint8_t  low_percent;
            uint8_t  mid_percent;
            uint8_t  high_percent;
        };
        CloudCover clouds;
    };
    CurrentConditions current;
    
    struct HourlyForecast {
        uint32_t time_offset_hours;
        float    temperature_c;
        float    precipitation_probability;
        float    precipitation_mm;
        float    wind_speed_mps;
        uint8_t  cloud_cover_percent;
        TinyStr  conditions;
    };
    std::array<HourlyForecast, kMaxHourlyForecasts> hourly;
    size_t hourly_count;
    
    struct Alert {
        SmallStr event;
        uint64_t start_time;
        uint64_t end_time;
        TinyStr  severity;
        MediumStr description;
    };
    std::array<Alert, kMaxAlerts> alerts;
    size_t alerts_count;
};

// Model 13: DatabaseQuery - Database query structure
struct DatabaseQuery {
    static constexpr std::size_t kMaxColumns = 16;
    static constexpr std::size_t kMaxConditions = 8;
    static constexpr std::size_t kMaxJoins = 4;
    
    LargeStr  query_id;
    uint64_t  timestamp_ms;
    TinyStr   operation;  // "SELECT", "INSERT", "UPDATE", "DELETE"
    
    struct Table {
        MediumStr name;
        MediumStr schema;
        MediumStr alias;
        
        struct Index {
            SmallStr name;
            TinyStr  type;
            bool     unique;
        };
        std::array<Index, 4> indexes;
        size_t indexes_count;
    };
    Table table;
    
    struct Column {
        SmallStr name;
        TinyStr  type;
        bool     nullable;
        std::optional<SmallStr> alias;
        
        struct Aggregation {
            TinyStr function;  // "SUM", "AVG", "COUNT", "MAX", "MIN"
            bool    distinct;
        };
        std::optional<Aggregation> aggregation;
    };
    std::array<Column, kMaxColumns> columns;
    size_t columns_count;
    
    struct Condition {
        SmallStr left_operand;
        TinyStr  operator_type;  // "=", "!=", "<", ">", "LIKE", "IN"
        SmallStr right_operand;
        TinyStr  logical_connector;  // "AND", "OR"
    };
    std::array<Condition, kMaxConditions> where_conditions;
    size_t where_conditions_count;
    
    struct Join {
        Table    joined_table;
        TinyStr  join_type;  // "INNER", "LEFT", "RIGHT", "FULL"
        SmallStr on_left;
        SmallStr on_right;
    };
    std::array<Join, kMaxJoins> joins;
    size_t joins_count;
    
    struct Options {
        uint32_t limit;
        uint32_t offset;
        SmallStr order_by;
        bool     ascending;
        std::optional<SmallStr> group_by;
    };
    std::optional<Options> options;
};

// Model 14: VideoStream - Video streaming metadata
struct VideoStream {
    static constexpr std::size_t kMaxTracks = 4;
    static constexpr std::size_t kMaxSegments = 16;
    
    LargeStr  stream_id;
    uint64_t  start_time_ms;
    uint32_t  duration_ms;
    
    struct Container {
        TinyStr  format;  // "mp4", "webm", "hls"
        MediumStr mime_type;
        uint32_t file_size_bytes;
        uint32_t bitrate_kbps;
    };
    Container container;
    
    struct VideoTrack {
        uint16_t width;
        uint16_t height;
        float    framerate;
        uint32_t bitrate_kbps;
        TinyStr  codec;
        TinyStr  profile;
        uint8_t  level;
        
        struct ColorInfo {
            TinyStr  primaries;
            TinyStr  transfer;
            TinyStr  matrix;
            bool     full_range;
        };
        ColorInfo color;
        
        struct Quality {
            float    psnr_db;
            float    ssim;
            uint32_t keyframe_interval;
            uint8_t  quality_level;
        };
        std::optional<Quality> quality;
    };
    std::optional<VideoTrack> video;
    
    struct AudioTrack {
        uint32_t sample_rate;
        uint8_t  channels;
        uint8_t  bit_depth;
        uint32_t bitrate_kbps;
        TinyStr  codec;
        TinyStr  language;
    };
    std::array<AudioTrack, kMaxTracks> audio_tracks;
    size_t audio_tracks_count;
    
    struct Segment {
        uint32_t sequence;
        uint32_t duration_ms;
        uint32_t size_bytes;
        MediumStr url;
        uint32_t start_offset_ms;
    };
    std::array<Segment, kMaxSegments> segments;
    size_t segments_count;
    
    struct Metadata {
        MediumStr title;
        SmallStr  language;
        uint64_t  creation_time;
        std::optional<MediumStr> thumbnail_url;
    };
    Metadata metadata;
};

// Model 15: EncryptionContext - Cryptography/security context
struct EncryptionContext {
    static constexpr std::size_t kMaxKeys = 4;
    static constexpr std::size_t kMaxCertificates = 3;
    
    LargeStr  session_id;
    uint64_t  created_timestamp;
    uint64_t  expires_timestamp;
    
    struct Algorithm {
        SmallStr name;
        TinyStr  mode;
        uint16_t key_size_bits;
        uint16_t block_size_bits;
        TinyStr  padding;
        
        struct Parameters {
            std::array<uint8_t, 16> iv;
            std::optional<std::array<uint8_t, 16>> salt;
            std::optional<uint32_t> iterations;
        };
        Parameters params;
    };
    Algorithm cipher;
    
    struct Key {
        MediumStr key_id;
        TinyStr   type;  // "symmetric", "public", "private"
        std::array<uint8_t, 64> material;
        uint16_t  material_length;
        
        struct Derivation {
            TinyStr  function;  // "PBKDF2", "HKDF", "scrypt"
            std::array<uint8_t, 32> salt;
            uint32_t iterations;
            SmallStr info;
        };
        std::optional<Derivation> derivation;
    };
    std::array<Key, kMaxKeys> keys;
    size_t keys_count;
    
    struct Certificate {
        MediumStr subject;
        MediumStr issuer;
        uint64_t  valid_from;
        uint64_t  valid_until;
        HugeStr   fingerprint;
        TinyStr   signature_algorithm;
    };
    std::array<Certificate, kMaxCertificates> certificates;
    size_t certificates_count;
    
    struct Integrity {
        TinyStr  hash_algorithm;
        HugeStr  hash_value;
        HugeStr  signature;
        bool     verified;
    };
    std::optional<Integrity> integrity;
};

// Model 16: MeshNode - Mesh network node
struct MeshNode {
    static constexpr std::size_t kMaxNeighbors = 8;
    static constexpr std::size_t kMaxRoutes = 12;
    
    MediumStr node_id;
    uint64_t  timestamp_ms;
    
    struct Hardware {
        SmallStr  chip_id;
        TinyStr   radio_type;  // "ble", "zigbee", "thread", "lora"
        int8_t    tx_power_dbm;
        uint16_t  frequency_mhz;
        
        struct Battery {
            uint16_t voltage_mv;
            uint8_t  level_percent;
            uint32_t remaining_mah;
            bool     charging;
        };
        std::optional<Battery> battery;
    };
    Hardware hardware;
    
    struct Neighbor {
        MediumStr node_id;
        int8_t    rssi_dbm;
        uint8_t   lqi;  // Link Quality Indicator
        uint16_t  hops;
        uint32_t  last_seen_ms;
        
        struct LinkStats {
            uint32_t packets_sent;
            uint32_t packets_received;
            uint32_t packets_lost;
            float    success_rate;
        };
        LinkStats stats;
    };
    std::array<Neighbor, kMaxNeighbors> neighbors;
    size_t neighbors_count;
    
    struct Route {
        MediumStr destination;
        MediumStr next_hop;
        uint8_t   metric;
        uint16_t  hops;
        uint32_t  age_ms;
        bool      active;
    };
    std::array<Route, kMaxRoutes> routing_table;
    size_t routing_table_count;
    
    struct Status {
        TinyStr  state;  // "active", "sleep", "offline"
        uint32_t uptime_s;
        uint32_t message_count;
        float    duty_cycle_percent;
    };
    Status status;
};

// Model 17: GameState - Game state snapshot
struct GameState {
    static constexpr std::size_t kMaxPlayers = 8;
    static constexpr std::size_t kMaxEntities = 32;
    
    uint64_t  game_id;
    uint64_t  timestamp_ms;
    uint32_t  tick;
    TinyStr   phase;  // "lobby", "playing", "paused", "ended"
    
    struct Player {
        MediumStr player_id;
        SmallStr  username;
        uint8_t   team;
        
        struct Stats {
            uint32_t score;
            uint16_t kills;
            uint16_t deaths;
            uint16_t assists;
            float    accuracy_percent;
            uint32_t damage_dealt;
            uint32_t damage_taken;
        };
        Stats stats;
        
        struct Transform {
            std::array<float, 3> position;
            std::array<float, 3> rotation;
            float    velocity_x;
            float    velocity_y;
            float    velocity_z;
        };
        Transform transform;
        
        struct State {
            uint16_t health;
            uint16_t max_health;
            uint16_t armor;
            uint8_t  level;
            uint32_t experience;
            bool     alive;
        };
        State state;
    };
    std::array<Player, kMaxPlayers> players;
    size_t players_count;
    
    struct Entity {
        uint32_t entity_id;
        TinyStr  type;
        std::array<float, 3> position;
        std::array<float, 3> rotation;
        bool     active;
        std::optional<uint32_t> owner_id;
    };
    std::array<Entity, kMaxEntities> entities;
    size_t entities_count;
    
    struct World {
        SmallStr  map_name;
        TinyStr   weather;
        float     time_of_day;
        uint32_t  seed;
    };
    World world;
};

// Model 18: LogEntry - Structured logging
struct LogEntry {
    static constexpr std::size_t kMaxFields = 12;
    static constexpr std::size_t kMaxStackFrames = 8;
    
    uint64_t  timestamp_ns;
    LargeStr  message;
    TinyStr   level;  // "trace", "debug", "info", "warn", "error", "fatal"
    SmallStr  logger_name;
    
    struct Source {
        MediumStr file;
        uint32_t  line;
        SmallStr  function;
        SmallStr  module;
        
        struct Process {
            uint32_t pid;
            MediumStr name;
            SmallStr  version;
        };
        Process process;
        
        struct Thread {
            uint64_t tid;
            SmallStr name;
        };
        Thread thread;
    };
    Source source;
    
    struct Field {
        SmallStr key;
        SmallStr value;
        TinyStr  type;  // "string", "int", "float", "bool"
    };
    std::array<Field, kMaxFields> fields;
    size_t fields_count;
    
    struct Exception {
        SmallStr  type;
        MediumStr message;
        
        struct StackFrame {
            MediumStr file;
            uint32_t  line;
            SmallStr  function;
            SmallStr  module;
        };
        std::array<StackFrame, kMaxStackFrames> stack_trace;
        size_t stack_trace_count;
        
        std::optional<MediumStr> cause;
    };
    std::optional<Exception> exception;
    
    struct Context {
        LargeStr  trace_id;
        LargeStr  span_id;
        std::optional<LargeStr> parent_span_id;
        SmallStr  service_name;
    };
    std::optional<Context> trace_context;
};

// Model 19: CalendarEvent - Calendar/scheduling event
struct CalendarEvent {
    static constexpr std::size_t kMaxAttendees = 16;
    static constexpr std::size_t kMaxReminders = 4;
    
    LargeStr  event_id;
    MediumStr title;
    LargeStr  description;
    
    struct DateTime {
        uint64_t timestamp;
        SmallStr timezone;
        bool     all_day;
    };
    DateTime start;
    DateTime end;
    
    struct Recurrence {
        TinyStr  frequency;  // "daily", "weekly", "monthly", "yearly"
        uint16_t interval;
        uint16_t count;
        std::optional<uint64_t> until_timestamp;
        
        struct ByRule {
            std::array<uint8_t, 7> by_weekday;
            size_t by_weekday_count;
            std::array<uint8_t, 12> by_month;
            size_t by_month_count;
            std::array<int8_t, 31> by_monthday;
            size_t by_monthday_count;
        };
        std::optional<ByRule> by_rules;
    };
    std::optional<Recurrence> recurrence;
    
    struct Location {
        MediumStr name;
        LargeStr  address;
        float     latitude;
        float     longitude;
        std::optional<MediumStr> room;
    };
    std::optional<Location> location;
    
    struct Attendee {
        MediumStr email;
        SmallStr  name;
        TinyStr   role;  // "required", "optional", "chair"
        TinyStr   status;  // "accepted", "declined", "tentative", "needs-action"
        bool      organizer;
    };
    std::array<Attendee, kMaxAttendees> attendees;
    size_t attendees_count;
    
    struct Reminder {
        uint32_t minutes_before;
        TinyStr  method;  // "email", "popup", "sms"
    };
    std::array<Reminder, kMaxReminders> reminders;
    size_t reminders_count;
    
    struct Metadata {
        MediumStr created_by;
        uint64_t  created_timestamp;
        uint64_t  modified_timestamp;
        TinyStr   visibility;  // "public", "private", "confidential"
        TinyStr   status;  // "confirmed", "tentative", "cancelled"
    };
    Metadata metadata;
};

// Model 20: HardwareProfile - Hardware configuration profile
struct HardwareProfile {
    static constexpr std::size_t kMaxGPUs = 4;
    static constexpr std::size_t kMaxDrives = 8;
    static constexpr std::size_t kMaxNetworkAdapters = 4;
    
    LargeStr  profile_id;
    MediumStr profile_name;
    uint64_t  timestamp;
    
    struct CPU {
        MediumStr model;
        SmallStr  vendor;
        uint8_t   cores;
        uint8_t   threads;
        uint32_t  base_freq_mhz;
        uint32_t  max_freq_mhz;
        
        struct Cache {
            uint32_t l1_data_kb;
            uint32_t l1_inst_kb;
            uint32_t l2_kb;
            uint32_t l3_kb;
        };
        Cache cache;
        
        struct Features {
            bool sse;
            bool sse2;
            bool avx;
            bool avx2;
            bool avx512;
            bool aes_ni;
        };
        Features features;
    };
    CPU cpu;
    
    struct Memory {
        uint64_t total_bytes;
        uint16_t frequency_mhz;
        TinyStr  type;  // "DDR3", "DDR4", "DDR5"
        uint8_t  channels;
        
        struct Timing {
            uint8_t cl;
            uint8_t trcd;
            uint8_t trp;
            uint8_t tras;
        };
        std::optional<Timing> timing;
    };
    Memory memory;
    
    struct GPU {
        MediumStr model;
        SmallStr  vendor;
        uint64_t  vram_bytes;
        uint16_t  core_clock_mhz;
        uint16_t  memory_clock_mhz;
        uint16_t  cuda_cores;
        uint8_t   compute_capability_major;
        uint8_t   compute_capability_minor;
    };
    std::array<GPU, kMaxGPUs> gpus;
    size_t gpus_count;
    
    struct Drive {
        MediumStr model;
        TinyStr   type;  // "hdd", "ssd", "nvme"
        uint64_t  capacity_bytes;
        uint32_t  rpm;
        TinyStr   interface;  // "sata", "pcie", "usb"
        
        struct Health {
            uint8_t  wear_level_percent;
            uint32_t power_on_hours;
            uint64_t bytes_written;
            uint8_t  temperature_c;
        };
        std::optional<Health> health;
    };
    std::array<Drive, kMaxDrives> drives;
    size_t drives_count;
    
    struct NetworkAdapter {
        SmallStr  name;
        TinyStr   type;  // "ethernet", "wifi", "bluetooth"
        MediumStr mac_address;
        uint32_t  speed_mbps;
        bool      wireless;
    };
    std::array<NetworkAdapter, kMaxNetworkAdapters> network_adapters;
    size_t network_adapters_count;
};

} // namespace code_size_test
