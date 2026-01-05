// Compile-time JSON Serialization Size Estimator Demo
//
// Demonstrates:
// - Compile-time estimation of maximum JSON output size
// - No runtime overhead - pure compile-time calculation
// - Conservative upper bounds for buffer allocation
// - Only fixed-size types are supported (std::array, FixedMap, not std::vector/std::string/std::map)

#include <JsonFusion/serialize_size_estimator.hpp>
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>
#include <JsonFusion/validators.hpp>
#include <array>
#include <optional>
#include <cstdint>
#include <utility>

using namespace JsonFusion;
using namespace JsonFusion::options;
using namespace JsonFusion::validators;

// Fixed-size map based on std::array<std::pair<K,V>, N>
// Satisfies MapReadable concept for serialization and size estimation
template<typename K, typename V, std::size_t N>
struct FixedMap {
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<K, V>;
    using storage_type = std::array<value_type, N>;
    using iterator = typename storage_type::iterator;
    using const_iterator = typename storage_type::const_iterator;
    
    storage_type data{};
    std::size_t count{0};  // Number of valid entries
    
    constexpr FixedMap() = default;
    
    constexpr iterator begin() { return data.begin(); }
    constexpr const_iterator begin() const { return data.begin(); }
    
    constexpr iterator end() { return data.begin() + count; }
    constexpr const_iterator end() const { return data.begin() + count; }
    
    constexpr std::size_t size() const { return count; }
    constexpr std::size_t capacity() const { return N; }
};

// Specialize std::tuple_size for FixedMap to enable compile-time size estimation
namespace std {
    template<typename K, typename V, std::size_t N>
    struct tuple_size<FixedMap<K, V, N>> 
        : std::integral_constant<std::size_t, N> {};
}

// Simple config example
struct SimpleConfig {
    std::array<char, 32> device_name{};
    std::uint16_t version{0};
    bool enabled{false};
};

// Nested config with arrays
struct NestedConfig {
    std::array<char, 16> name{};
    A<std::uint16_t, range<0, 65535>> port{8080};
    
    struct Endpoint {
        std::array<char, 24> address{};
        std::uint16_t port{0};
    };
    
    std::array<Endpoint, 4> endpoints{};
};

// Complex config with optionals and validation
struct ComplexConfig {
    std::array<char, 64> app_name{};
    A<std::array<char, 16>> version{};
    std::optional<std::uint32_t> build_number{};
    
    struct Server {
        A<std::array<char, 32>> hostname{};
        A<std::uint16_t, range<1024, 65535>> port{8080};
        A<bool> ssl_enabled{false};
    };
    
    A<std::array<Server, 8>> servers{};
    
    // FixedMap example: compile-time sized map
    FixedMap<int, std::array<char, 32>, 16> id_to_description{};
    
    struct NamedValue {
        std::array<char, 16> name{};
        int value{};
    };
    std::array<NamedValue, 10> named_values{};
};

// Compile-time size estimations
static_assert([] {
    constexpr std::size_t simple_size = size_estimator::EstimateMaxSerializedSize<SimpleConfig>();
    constexpr std::size_t nested_size = size_estimator::EstimateMaxSerializedSize<NestedConfig>();
    constexpr std::size_t complex_size = size_estimator::EstimateMaxSerializedSize<ComplexConfig>();
    
    // Verify sizes are reasonable (not zero, not absurdly large)
    return simple_size > 0 && simple_size < 1000 &&
           nested_size > 0 && nested_size < 10000 &&
           complex_size > 0 && complex_size < 100000;
}());

// Practical usage: allocate exactly the right size buffer
constexpr bool test_with_estimated_buffer() {
    SimpleConfig config{};
    config.device_name = std::array<char, 32>{'T', 'e', 's', 't', '\0'};
    config.version = 42;
    config.enabled = true;
    
    // Get compile-time size estimate
    constexpr std::size_t max_size = size_estimator::EstimateMaxSerializedSize<SimpleConfig>();
    
    // Allocate buffer based on estimate
    std::array<char, max_size> buffer{};
    
    // Serialize
    auto res = Serialize(config, buffer.data(), buffer.data() + buffer.size() - 1);
    if (!res) return false;
    
    // Verify actual size is within estimate
    std::size_t actual_size = res.bytesWritten();
    if (actual_size >= max_size) return false; // Should never happen
    
    // Null-terminate and verify it's valid JSON
    buffer[actual_size] = '\0';
    
    // Parse it back to verify
    SimpleConfig config2{};
    if (!Parse(config2, buffer.data())) return false;
    
    return config2.version == 42 && config2.enabled;
}

static_assert(test_with_estimated_buffer(), "Estimated buffer test failed");

// Display sizes at compile time (will show in compiler output if we use them)
template<std::size_t N>
struct CompileTimeSize {
    static constexpr std::size_t value = N;
};

// Instantiate to see sizes (use -Wtemplate-backtrace-limit=0 to see all)
using SimpleSize = CompileTimeSize<size_estimator::EstimateMaxSerializedSize<SimpleConfig>()>;
using NestedSize = CompileTimeSize<size_estimator::EstimateMaxSerializedSize<NestedConfig>()>;
using ComplexSize = CompileTimeSize<size_estimator::EstimateMaxSerializedSize<ComplexConfig>()>;

// Demonstrate real-world usage
constexpr auto calculate_buffer_sizes() {
    struct BufferSizes {
        std::size_t simple;
        std::size_t nested;
        std::size_t complex;
    };
    
    return BufferSizes{
        size_estimator::EstimateMaxSerializedSize<SimpleConfig>(),
        size_estimator::EstimateMaxSerializedSize<NestedConfig>(),
        size_estimator::EstimateMaxSerializedSize<ComplexConfig>()
    };
}

constexpr auto buffer_sizes = calculate_buffer_sizes();


