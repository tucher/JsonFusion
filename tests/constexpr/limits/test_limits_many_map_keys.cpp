#include "../test_helpers.hpp"
#include <JsonFusion/parser.hpp>
#include <JsonFusion/validators.hpp>
#include <JsonFusion/options.hpp>
#include <JsonFusion/static_schema.hpp>
#include <array>

using namespace TestHelpers;
using namespace JsonFusion;
using namespace JsonFusion::validators;
using namespace JsonFusion::static_schema;
using JsonFusion::A;

// ============================================================================
// Test: Many Map Keys Limits
// ============================================================================

// Map entry structure required by ConsumingMapStreamerLike
template<typename K, typename V>
struct MapEntry {
    K key;
    V value;
};

// Simple map consumer for testing many keys
struct ManyKeysConsumer {
    using value_type = MapEntry<std::array<char, 32>, int>;
    
    std::array<MapEntry<std::array<char, 32>, int>, 50> entries{};
    std::size_t count = 0;
    
    constexpr bool consume(const value_type& entry) {
        if (count >= 50) return false;
        entries[count++] = entry;
        return true;
    }
    
    constexpr bool finalize(bool success) {
        return success;
    }
    
    constexpr void reset() {
        count = 0;
    }
    
    // Helper to find entry by key
    constexpr int find_value(const char* key_name) const {
        for (std::size_t i = 0; i < count; ++i) {
            std::size_t j = 0;
            while (j < 32 && entries[i].key[j] != '\0' && key_name[j] != '\0') {
                if (entries[i].key[j] != key_name[j]) break;
                ++j;
            }
            if (entries[i].key[j] == '\0' && key_name[j] == '\0') {
                return entries[i].value;
            }
        }
        return -1;
    }
};

static_assert(ConsumingMapStreamerLike<ManyKeysConsumer>);
static_assert(ParsableMapLike<ManyKeysConsumer>);

// Test 1: allowed_keys with 30 keys
struct ManyAllowedKeys {
    ManyKeysConsumer config;
};

constexpr bool test_many_allowed_keys() {
    A<ManyAllowedKeys, allowed_keys<
        "key0", "key1", "key2", "key3", "key4", "key5", "key6", "key7", "key8", "key9",
        "key10", "key11", "key12", "key13", "key14", "key15", "key16", "key17", "key18", "key19",
        "key20", "key21", "key22", "key23", "key24", "key25", "key26", "key27", "key28", "key29"
    >> obj{};
    
    constexpr std::string_view json = R"({"config":{"key0":0,"key1":1,"key2":2,"key3":3,"key4":4,"key5":5,"key6":6,"key7":7,"key8":8,"key9":9,"key10":10,"key11":11,"key12":12,"key13":13,"key14":14,"key15":15,"key16":16,"key17":17,"key18":18,"key19":19,"key20":20,"key21":21,"key22":22,"key23":23,"key24":24,"key25":25,"key26":26,"key27":27,"key28":28,"key29":29}})";
    
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    
    // Verify some keys
    return obj.get().config.find_value("key0") == 0
        && obj.get().config.find_value("key15") == 15
        && obj.get().config.find_value("key29") == 29
        && obj.get().config.count == 30;
}
static_assert(test_many_allowed_keys(), "allowed_keys with 30 keys");

// Test 2: required_keys with many keys
struct ManyRequiredKeys {
    ManyKeysConsumer data;
};

constexpr bool test_many_required_keys() {
    A<ManyRequiredKeys, required_keys<
        "req0", "req1", "req2", "req3", "req4", "req5", "req6", "req7", "req8", "req9",
        "req10", "req11", "req12", "req13", "req14", "req15", "req16", "req17", "req18", "req19"
    >> obj{};
    
    constexpr std::string_view json = R"({"data":{"req0":0,"req1":2,"req2":4,"req3":6,"req4":8,"req5":10,"req6":12,"req7":14,"req8":16,"req9":18,"req10":20,"req11":22,"req12":24,"req13":26,"req14":28,"req15":30,"req16":32,"req17":34,"req18":36,"req19":38}})";
    
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    
    return obj.get().data.find_value("req0") == 0
        && obj.get().data.find_value("req10") == 20
        && obj.get().data.find_value("req19") == 38
        && obj.get().data.count == 20;
}
static_assert(test_many_required_keys(), "required_keys with 20 keys");

// Test 3: forbidden_keys with many keys
struct ManyForbiddenKeys {
    ManyKeysConsumer safe;
};

constexpr bool test_many_forbidden_keys() {
    A<ManyForbiddenKeys, forbidden_keys<
        "bad0", "bad1", "bad2", "bad3", "bad4", "bad5", "bad6", "bad7", "bad8", "bad9",
        "bad10", "bad11", "bad12", "bad13", "bad14", "bad15", "bad16", "bad17", "bad18", "bad19",
        "bad20", "bad21", "bad22", "bad23", "bad24", "bad25", "bad26", "bad27", "bad28", "bad29"
    >> obj{};
    
    // Use allowed keys that are not forbidden
    constexpr std::string_view json = R"({"safe":{"good0":1,"good1":2,"good2":3}})";
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    
    return obj.get().safe.find_value("good0") == 1
        && obj.get().safe.find_value("good2") == 3
        && obj.get().safe.count == 3;
}
static_assert(test_many_forbidden_keys(), "forbidden_keys with 30 keys");

// Test 4: Combined validators with many keys
struct CombinedManyKeys {
    ManyKeysConsumer config;
};

constexpr bool test_combined_many_keys() {
    A<CombinedManyKeys, 
        required_keys<"req0", "req1", "req2", "req3", "req4">,
        allowed_keys<
            "req0", "req1", "req2", "req3", "req4",
            "opt0", "opt1", "opt2", "opt3", "opt4",
            "opt5", "opt6", "opt7", "opt8", "opt9"
        >,
        min_properties<5>,
        max_properties<15>
    > obj{};
    
    constexpr std::string_view json = R"({"config":{"req0":0,"req1":1,"req2":2,"req3":3,"req4":4,"opt0":100,"opt1":101,"opt2":102,"opt3":103,"opt4":104,"opt5":105,"opt6":106,"opt7":107,"opt8":108,"opt9":109}})";
    
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    
    return obj.get().config.count == 15  // 5 required + 10 optional
        && obj.get().config.find_value("req0") == 0
        && obj.get().config.find_value("opt9") == 109;
}
static_assert(test_combined_many_keys(), "Combined validators with many keys");

// Test 5: Binary search threshold test (many keys should use binary search)
struct BinarySearchTest {
    ManyKeysConsumer data;
};

constexpr bool test_binary_search_threshold() {
    // Test with enough keys to trigger binary search (typically > 8-10 keys)
    A<BinarySearchTest, allowed_keys<
        "key00", "key01", "key02", "key03", "key04", "key05", "key06", "key07",
        "key08", "key09", "key10", "key11", "key12", "key13", "key14", "key15"
    >> obj{};
    
    constexpr std::string_view json = R"({"data":{"key00":0,"key01":1,"key02":2,"key03":3,"key04":4,"key05":5,"key06":6,"key07":7,"key08":8,"key09":9,"key10":10,"key11":11,"key12":12,"key13":13,"key14":14,"key15":15}})";
    
    auto result = JsonFusion::Parse(obj, json);
    
    if (!result) return false;
    
    // Verify keys at beginning, middle, and end (binary search should handle all)
    return obj.get().data.find_value("key00") == 0
        && obj.get().data.find_value("key08") == 8
        && obj.get().data.find_value("key15") == 15
        && obj.get().data.count == 16;
}
static_assert(test_binary_search_threshold(), "Binary search threshold with 16 keys");
