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
#include <nlohmann/json.hpp>

using std::array;
using std::int64_t;
using std::list;
using std::optional;
using std::string;
using std::string_view;
using std::vector;

namespace rj = rapidjson;
namespace nl = nlohmann;

// ----------------------
// Test structures
// ----------------------

struct Limits {
    int minValue;
    int maxValue;
};

struct FixedStrings {
    array<char, 8>  code{};   // e.g. "CFG001"
    array<char, 16> label{};  // e.g. "MainConfig"
};

struct Node {
    string                     name;
    bool                       active;
    vector<int>                weights;
    optional<double>           bias;
    vector<optional<bool>>     flags;
    vector<Node>               children;
};

struct Credentials {
    string            user;
    optional<string>  password;
};

struct ComplexConfig {
    // Scalars
    bool         enabled;
    char         mode;             // parsed as number then cast
    int          retryCount;
    double       timeoutSeconds;

    // Strings
    string       title;
    FixedStrings fixedStrings;

    // Arrays
    array<int, 3>          rgb;
    vector<string>         tags;
    list<int64_t>          counters;
    vector<vector<int>>    matrix;

    // Optionals
    optional<int>          debugLevel;
    optional<string>       optionalNote;
    optional<vector<int>>  optionalArray;
    optional<Limits>       optionalLimits;

    // Nested objects
    Limits                 hardLimits;
    Credentials            creds;

    // Recursive graph
    Node                   rootNode;
    vector<Node>           extraNodes;
    optional<Node>         optionalNode;
    vector<optional<Node>> nodeHistory;
};


namespace static_containers_variant {
// using string = std::array<char, 100>;

// template <class T>
// using vector = std::array<T, 100>;

// template <class T>
// using list = std::array<T, 100>;

struct Limits {
    int minValue;
    int maxValue;
};

struct FixedStrings {
    array<char, 8>  code{};   // e.g. "CFG001"
    array<char, 16> label{};  // e.g. "MainConfig"
};

struct Node {
    string                     name;
    bool                       active;
    vector<int>                weights;
    optional<double>           bias;
    vector<optional<bool>>     flags;
    vector<Node>               children;
};

struct Credentials {
    string            user;
    optional<string>  password;
};

struct ComplexConfig {
    // Scalars
    bool         enabled;
    char         mode;             // parsed as number then cast
    int          retryCount;
    double       timeoutSeconds;

    // Strings
    string       title;
    FixedStrings fixedStrings;

    // Arrays
    array<int, 3>          rgb;
    vector<string>         tags;
    list<int64_t>          counters;
    vector<vector<int>>    matrix;

    // Optionals
    optional<int>          debugLevel;
    optional<string>       optionalNote;
    optional<vector<int>>  optionalArray;
    optional<Limits>       optionalLimits;

    // Nested objects
    Limits                 hardLimits;
    Credentials            creds;

    // Recursive graph
    Node                   rootNode;
    vector<Node>           extraNodes;
    optional<Node>         optionalNode;
    vector<optional<Node>> nodeHistory;
};
}

// ----------------------
// JSON under test
// ----------------------

static const char kJson[] = R"json(
{
  "enabled": true,
  "mode": 1,
  "retryCount": 3,
  "timeoutSeconds": 1.5,

  "title": "Main \\\"config\\\" example",

  "fixedStrings": {
    "code": "CFG001",
    "label": "MainConfig"
  },

  "rgb": [255, 128, 64],

  "tags": [
    "alpha",
    "beta",
    "gamma"
  ],

  "counters": [
    1,
    2,
    9999999999
  ],

  "matrix": [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
  ],

  "debugLevel": 42,
  "optionalNote": null,
  "optionalArray": [10, 20, 30],
  "optionalLimits": {
    "minValue": 5,
    "maxValue": 95
  },

  "hardLimits": {
    "minValue": 0,
    "maxValue": 100
  },

  "creds": {
    "user": "admin",
    "password": "secret"
  },

  "rootNode": {
    "name": "root",
    "active": true,
    "weights": [1, 2, 3, 4],
    "bias": 0.5,
    "flags": [true, null, false],
    "children": [
      {
        "name": "child1",
        "active": false,
        "weights": [],
        "bias": null,
        "flags": [],
        "children": []
      },
      {
        "name": "child2",
        "active": true,
        "weights": [5, 6],
        "bias": -1.25,
        "flags": [null, true],
        "children": []
      }
    ]
  },

  "extraNodes": [
    {
      "name": "extra1",
      "active": true,
      "weights": [10],
      "bias": null,
      "flags": [false],
      "children": []
    },
    {
      "name": "extra2",
      "active": false,
      "weights": [],
      "bias": 3.1415,
      "flags": [],
      "children": []
    }
  ],

  "optionalNode": {
    "name": "optional",
    "active": true,
    "weights": [7, 8, 9],
    "bias": 2.718,
    "flags": [false, true],
    "children": []
  },

  "nodeHistory": [
    null,
    {
      "name": "history1",
      "active": false,
      "weights": [100],
      "bias": null,
      "flags": [],
      "children": []
    },
    null
  ]
}
)json";

// ----------------------
// RapidJSON -> struct mapping
// ----------------------

static bool FillLimits(const rj::Value& v, Limits& out) {
    if (!v.IsObject()) return false;
    out.minValue = v["minValue"].GetInt();
    out.maxValue = v["maxValue"].GetInt();
    return true;
}

static bool FillNode(const rj::Value& v, Node& out);

static bool FillNodeArray(const rj::Value& arr, vector<Node>& out) {
    if (!arr.IsArray()) return false;
    out.clear();
    out.resize(arr.Size());
    for (rj::SizeType i = 0; i < arr.Size(); ++i) {
        if (!FillNode(arr[i], out[i])) return false;
    }
    return true;
}

static bool FillNode(const rj::Value& v, Node& out) {
    if (!v.IsObject()) return false;

    out.name   = v["name"].GetString();
    out.active = v["active"].GetBool();

    const auto& w = v["weights"];
    out.weights.clear();
    if (!w.IsArray()) return false;
    for (auto& x : w.GetArray()) {
        out.weights.push_back(x.GetInt());
    }

    const auto& bias = v["bias"];
    if (bias.IsNull()) {
        out.bias.reset();
    } else {
        out.bias = bias.GetDouble();
    }

    const auto& flags = v["flags"];
    out.flags.clear();
    if (!flags.IsArray()) return false;
    for (auto& fv : flags.GetArray()) {
        if (fv.IsNull()) {
            out.flags.emplace_back(std::nullopt);
        } else {
            out.flags.emplace_back(fv.GetBool());
        }
    }

    const auto& children = v["children"];
    return FillNodeArray(children, out.children);
}

static bool FillComplexConfig(const rj::Document& doc, ComplexConfig& out) {
    if (!doc.IsObject()) return false;
    const auto& v = doc;

    // Scalars
    out.enabled       = v["enabled"].GetBool();
    out.mode          = static_cast<char>(v["mode"].GetInt());
    out.retryCount    = v["retryCount"].GetInt();
    out.timeoutSeconds= v["timeoutSeconds"].GetDouble();

    // Strings
    out.title         = v["title"].GetString();

    const auto& fs = v["fixedStrings"];
    {
        const char* code = fs["code"].GetString();
        const char* label= fs["label"].GetString();
        std::snprintf(out.fixedStrings.code.data(),  out.fixedStrings.code.size(),  "%s", code);
        std::snprintf(out.fixedStrings.label.data(), out.fixedStrings.label.size(), "%s", label);
    }

    // Arrays
    const auto& rgb = v["rgb"];
    for (int i = 0; i < 3; ++i) {
        out.rgb[i] = rgb[i].GetInt();
    }

    const auto& tags = v["tags"];
    out.tags.clear();
    for (auto& tv : tags.GetArray()) {
        out.tags.emplace_back(tv.GetString());
    }

    const auto& counters = v["counters"];
    out.counters.clear();
    for (auto& cv : counters.GetArray()) {
        out.counters.push_back(cv.GetInt64());
    }

    const auto& matrix = v["matrix"];
    out.matrix.clear();
    for (auto& rowVal : matrix.GetArray()) {
        vector<int> row;
        for (auto& cell : rowVal.GetArray()) {
            row.push_back(cell.GetInt());
        }
        out.matrix.emplace_back(std::move(row));
    }

    // Optionals
    if (v.HasMember("debugLevel") && !v["debugLevel"].IsNull())
        out.debugLevel = v["debugLevel"].GetInt();
    else
        out.debugLevel.reset();

    if (v.HasMember("optionalNote") && !v["optionalNote"].IsNull())
        out.optionalNote = v["optionalNote"].GetString();
    else
        out.optionalNote.reset();

    if (v.HasMember("optionalArray") && !v["optionalArray"].IsNull()) {
        const auto& arr = v["optionalArray"];
        vector<int> tmp;
        for (auto& iv : arr.GetArray()) tmp.push_back(iv.GetInt());
        out.optionalArray = std::move(tmp);
    } else {
        out.optionalArray.reset();
    }

    if (v.HasMember("optionalLimits") && !v["optionalLimits"].IsNull()) {
        Limits lim;
        if (!FillLimits(v["optionalLimits"], lim)) return false;
        out.optionalLimits = lim;
    } else {
        out.optionalLimits.reset();
    }

    // Nested objects
    if (!FillLimits(v["hardLimits"], out.hardLimits)) return false;

    const auto& creds = v["creds"];
    out.creds.user = creds["user"].GetString();
    if (creds.HasMember("password") && !creds["password"].IsNull())
        out.creds.password = creds["password"].GetString();
    else
        out.creds.password.reset();

    // Recursive
    if (!FillNode(v["rootNode"], out.rootNode)) return false;

    const auto& extraNodes = v["extraNodes"];
    out.extraNodes.clear();
    out.extraNodes.resize(extraNodes.Size());
    for (rj::SizeType i = 0; i < extraNodes.Size(); ++i) {
        if (!FillNode(extraNodes[i], out.extraNodes[i])) return false;
    }

    const auto& optionalNode = v["optionalNode"];
    if (optionalNode.IsNull()) {
        out.optionalNode.reset();
    } else {
        Node n;
        if (!FillNode(optionalNode, n)) return false;
        out.optionalNode = std::move(n);
    }

    const auto& history = v["nodeHistory"];
    out.nodeHistory.clear();
    for (auto& item : history.GetArray()) {
        if (item.IsNull()) {
            out.nodeHistory.emplace_back(std::nullopt);
        } else {
            Node n;
            if (!FillNode(item, n)) return false;
            out.nodeHistory.emplace_back(std::move(n));
        }
    }

    return true;
}


// ----------------------
// nlohmann::json -> struct mapping
// ----------------------

static bool FillLimits(const nl::json& j, Limits& out) {
    if (!j.is_object()) return false;
    out.minValue = j.at("minValue").get<int>();
    out.maxValue = j.at("maxValue").get<int>();
    return true;
}

static bool FillNode(const nl::json& j, Node& out);

static bool FillNodeArray(const nl::json& arr, vector<Node>& out) {
    if (!arr.is_array()) return false;
    out.clear();
    out.resize(arr.size());
    for (std::size_t i = 0; i < arr.size(); ++i) {
        if (!FillNode(arr[i], out[i])) return false;
    }
    return true;
}

static bool FillNode(const nl::json& j, Node& out) {
    if (!j.is_object()) return false;

    out.name   = j.at("name").get<string>();
    out.active = j.at("active").get<bool>();

    const auto& w = j.at("weights");
    if (!w.is_array()) return false;
    out.weights.clear();
    for (const auto& x : w) {
        out.weights.push_back(x.get<int>());
    }

    const auto& bias = j.at("bias");
    if (bias.is_null()) {
        out.bias.reset();
    } else {
        out.bias = bias.get<double>();
    }

    const auto& flags = j.at("flags");
    if (!flags.is_array()) return false;
    out.flags.clear();
    for (const auto& fv : flags) {
        if (fv.is_null()) {
            out.flags.emplace_back(std::nullopt);
        } else {
            out.flags.emplace_back(fv.get<bool>());
        }
    }

    const auto& children = j.at("children");
    return FillNodeArray(children, out.children);
}

static bool FillComplexConfig(const nl::json& j, ComplexConfig& out) {
    if (!j.is_object()) return false;

    // Scalars
    out.enabled        = j.at("enabled").get<bool>();
    out.mode           = static_cast<char>(j.at("mode").get<int>());
    out.retryCount     = j.at("retryCount").get<int>();
    out.timeoutSeconds = j.at("timeoutSeconds").get<double>();

    // Strings
    out.title          = j.at("title").get<string>();

    const auto& fs = j.at("fixedStrings");
    {
        const char* code  = fs.at("code").get_ref<const std::string&>().c_str();
        const char* label = fs.at("label").get_ref<const std::string&>().c_str();
        std::snprintf(out.fixedStrings.code.data(),
                      out.fixedStrings.code.size(), "%s", code);
        std::snprintf(out.fixedStrings.label.data(),
                      out.fixedStrings.label.size(), "%s", label);
    }

    // Arrays
    const auto& rgb = j.at("rgb");
    for (int i = 0; i < 3; ++i) {
        out.rgb[i] = rgb.at(i).get<int>();
    }

    const auto& tags = j.at("tags");
    out.tags.clear();
    for (const auto& tv : tags) {
        out.tags.emplace_back(tv.get<string>());
    }

    const auto& counters = j.at("counters");
    out.counters.clear();
    for (const auto& cv : counters) {
        out.counters.push_back(cv.get<int64_t>());
    }

    const auto& matrix = j.at("matrix");
    out.matrix.clear();
    for (const auto& rowVal : matrix) {
        vector<int> row;
        for (const auto& cell : rowVal) {
            row.push_back(cell.get<int>());
        }
        out.matrix.emplace_back(std::move(row));
    }

    // Optionals
    if (j.contains("debugLevel") && !j.at("debugLevel").is_null())
        out.debugLevel = j.at("debugLevel").get<int>();
    else
        out.debugLevel.reset();

    if (j.contains("optionalNote") && !j.at("optionalNote").is_null())
        out.optionalNote = j.at("optionalNote").get<string>();
    else
        out.optionalNote.reset();

    if (j.contains("optionalArray") && !j.at("optionalArray").is_null()) {
        const auto& arr = j.at("optionalArray");
        vector<int> tmp;
        for (const auto& iv : arr) tmp.push_back(iv.get<int>());
        out.optionalArray = std::move(tmp);
    } else {
        out.optionalArray.reset();
    }

    if (j.contains("optionalLimits") && !j.at("optionalLimits").is_null()) {
        Limits lim;
        if (!FillLimits(j.at("optionalLimits"), lim)) return false;
        out.optionalLimits = lim;
    } else {
        out.optionalLimits.reset();
    }

    // Nested objects
    if (!FillLimits(j.at("hardLimits"), out.hardLimits)) return false;

    const auto& creds = j.at("creds");
    out.creds.user = creds.at("user").get<string>();
    if (creds.contains("password") && !creds.at("password").is_null())
        out.creds.password = creds.at("password").get<string>();
    else
        out.creds.password.reset();

    // Recursive
    if (!FillNode(j.at("rootNode"), out.rootNode)) return false;

    const auto& extraNodes = j.at("extraNodes");
    out.extraNodes.clear();
    out.extraNodes.resize(extraNodes.size());
    for (std::size_t i = 0; i < extraNodes.size(); ++i) {
        if (!FillNode(extraNodes[i], out.extraNodes[i])) return false;
    }

    const auto& optionalNode = j.at("optionalNode");
    if (optionalNode.is_null()) {
        out.optionalNode.reset();
    } else {
        Node n;
        if (!FillNode(optionalNode, n)) return false;
        out.optionalNode = std::move(n);
    }

    const auto& history = j.at("nodeHistory");
    out.nodeHistory.clear();
    for (const auto& item : history) {
        if (item.is_null()) {
            out.nodeHistory.emplace_back(std::nullopt);
        } else {
            Node n;
            if (!FillNode(item, n)) return false;
            out.nodeHistory.emplace_back(std::move(n));
        }
    }

    return true;
}

// ----------------------
// Bench functions
// ----------------------

// 1) Your reflection-based parser
bool ParseWithReflection(ComplexConfig& cfg) {
    // Container can be std::string_view, std::string, etc.
    string_view sv{kJson, sizeof(kJson) - 1};
    auto res = JSONReflection2::Parse(cfg, sv);
    return static_cast<bool>(res);
}

// 2) RapidJSON: parse + map to struct
bool ParseWithRapidJSON(ComplexConfig& cfg) {
    rj::Document doc;
    doc.Parse(kJson);
    if (doc.HasParseError()) {
        std::cerr << "RapidJSON parse error: "
                  << rj::GetParseError_En(doc.GetParseError())
                  << " at offset " << doc.GetErrorOffset() << "\n";
        return false;
    }
    // return FillComplexConfig(doc, cfg);
    return true;
}


// 2) nlohmann::json: parse + map to struct
bool ParseWithNlohmann(ComplexConfig& cfg) {
    nl::json j;
    try {
        j = nl::json::parse(kJson, kJson + sizeof(kJson) - 1);
    } catch (const nl::json::parse_error& e) {
        std::cerr << "nlohmann::json parse error: " << e.what() << "\n";
        return false;
    }
    return FillComplexConfig(j, cfg);
}


// ----------------------
// Main benchmark
// ----------------------

int main() {
    constexpr int iterations = 2000000;

    std::ios::sync_with_stdio(false);

    ComplexConfig cfg;

    // Warmup
    if (!ParseWithReflection(cfg)) {
        std::cerr << "Reflection parser failed on warmup\n";
        return 1;
    }
    if (!ParseWithRapidJSON(cfg)) {
        std::cerr << "RapidJSON parser failed on warmup\n";
        return 1;
    }
    if (!ParseWithNlohmann(cfg)) {
        std::cerr << "nlohmann::json parser failed on warmup\n";
        return 1;
    }

    using clock = std::chrono::steady_clock;

    // Benchmark: JSONReflection2
    {
        auto start = clock::now();
        for (int i = 0; i < iterations; ++i) {
            ComplexConfig c;
            ParseWithReflection(c);
        }
        auto end = clock::now();
        auto total_us =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << "JSONReflection2: total " << total_us
                  << " us, avg " << (double)total_us / iterations << " us/parse\n";
    }

    // Benchmark: RapidJSON + mapping to struct
    {
        auto start = clock::now();
        for (int i = 0; i < iterations; ++i) {
            ComplexConfig c;
            ParseWithRapidJSON(c);
        }
        auto end = clock::now();
        auto total_us =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << "RapidJSON+mapping: total " << total_us
                  << " us, avg " << (double)total_us / iterations << " us/parse\n";
    }
    {
        auto start = clock::now();
        for (int i = 0; i < iterations; ++i) {
            ComplexConfig c;
            ParseWithNlohmann(c);
        }
        auto end = clock::now();
        auto total_us =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << "nlohmann::json+mapping: total " << total_us
                  << " us, avg " << (double)total_us / iterations << " us/parse\n";
    }

    return 0;
}
