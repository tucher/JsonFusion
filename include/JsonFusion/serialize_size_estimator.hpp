#pragma once

/// @file serialize_size_estimator.hpp
/// @brief Compile-time JSON serialization size estimation
///
/// This header provides compile-time estimation of the maximum serialized size
/// for JsonFusion types. The estimator uses C++ type constraints to ensure
/// exact upper bounds - it will either provide a precise estimate or fail at
/// compile time.
///
/// ## Supported Types
///
/// The estimator ONLY supports fixed-size types:
/// - **Primitives**: bool, integers, floating-point (max size is known)
/// - **Strings**: std::array<char, N> only (no std::string, no dynamic strings)
/// - **Arrays**: std::array<T, N> only (no std::vector, no dynamic arrays)
/// - **Maps**: Fixed-size map types with std::tuple_size (no std::map/std::unordered_map)
/// - **Optionals**: std::optional<T>, std::unique_ptr<T> (adds null overhead)
/// - **Structs**: All fields must be fixed-size
///
/// ## Validation Constraints are NOT Used
///
/// The estimator does NOT rely on validators like max_length<N>, max_items<N>,
/// or max_properties<N>. Instead, it uses only C++ type system constraints:
/// - String length from std::tuple_size<std::array<char, N>>
/// - Array size from std::tuple_size<std::array<T, N>>
/// - Map size from std::tuple_size (for fixed-size map structures)
///
/// This design ensures that the serializer cannot produce output larger than
/// the estimated size, because the C++ types themselves enforce the bounds.
///
/// ## Compile-Time Failures
///
/// Attempting to estimate size for unbounded types will fail at compile time:
/// ```cpp
/// struct Invalid {
///     std::string name;  // ERROR: only fixed-size arrays supported
///     std::vector<int> values;  // ERROR: only fixed-size arrays supported
/// };
/// constexpr auto size = EstimateMaxSerializedSize<Invalid>();  // Won't compile
/// ```
///
/// ## Example Usage
///
/// ```cpp
/// struct Config {
///     std::array<char, 32> device_name{};
///     std::uint16_t port{8080};
///     std::array<int, 10> values{};
/// };
///
/// // Get compile-time size estimate
/// constexpr std::size_t buffer_size = EstimateMaxSerializedSize<Config>();
///
/// // Allocate buffer at compile time
/// std::array<char, buffer_size> buffer{};
///
/// // Serialize with guaranteed no overflow
/// auto result = Serialize(config, buffer.data(), buffer.data() + buffer.size());
/// ```

#include "static_schema.hpp"
#include "options.hpp"
#include "struct_introspection.hpp"
#include "validators.hpp"
#include "serializer.hpp"
#include <type_traits>
#include <limits>
#include <cstdint>
#include <optional>
#include <memory>

namespace JsonFusion::size_estimator {

using namespace static_schema;
using namespace JsonFusion::validators::validators_detail;

namespace detail {

// Helper: count decimal digits in a number
constexpr std::size_t count_decimal_digits(std::size_t n) {
    if (n == 0) return 1;
    std::size_t count = 0;
    while (n > 0) {
        n /= 10;
        ++count;
    }
    return count;
}

// Calculate maximum digits for integer types
template<typename T>
constexpr std::size_t max_integer_digits() {
    if constexpr (std::is_signed_v<T>) {
        // -9223372036854775808 for int64_t (19 digits + sign)
        return count_decimal_digits(static_cast<std::size_t>(std::numeric_limits<T>::max())) + 1; // +1 for sign
    } else {
        // 18446744073709551615 for uint64_t (20 digits)
        return count_decimal_digits(std::numeric_limits<T>::max());
    }
}

// Calculate maximum size for floating point (conservative estimate)
// Format: [-]d.dddddde[+/-]ddd
// double: sign(1) + digits(17) + dot(1) + 'e'(1) + exp_sign(1) + exp(3) = 24
// float:  sign(1) + digits(9)  + dot(1) + 'e'(1) + exp_sign(1) + exp(2) = 15
template<typename T>
constexpr std::size_t max_float_size() {
    if constexpr (std::is_same_v<T, float>) {
        return 15;
    } else if constexpr (std::is_same_v<T, double>) {
        return 24;
    } else {
        return 32; // long double or other
    }
}

// Forward declaration
template <typename Type>
    requires SerializableValue<Type>
constexpr std::size_t EstimateMaxSizeImpl();

// Helper to calculate precise serialized size of a compile-time string
constexpr std::size_t CalculateSerializedStringSize(std::string_view str) {
    // Worst case: every char as \u00XX (6 bytes) + 2 quotes
    const std::size_t max_escaped_size = str.size() * 6 + 2;
    
    // We can't actually serialize at runtime in constexpr context without allocated buffer
    // For now, we'll use a simpler approach: count what would be escaped
    std::size_t size = 2; // opening and closing quotes
    
    for (std::size_t i = 0; i < str.size(); ++i) {
        unsigned char uc = static_cast<unsigned char>(str[i]);
        
        // Check if character needs escaping
        if (uc == '"' || uc == '\\' || uc == '\b' || uc == '\f' || 
            uc == '\n' || uc == '\r' || uc == '\t') {
            size += 2; // \X
        } else if (uc < 0x20) {
            size += 6; // \u00XX
        } else {
            size += 1; // regular character
        }
    }
    
    return size;
}

// Size for boolean: "true" or "false" → 5 bytes max
template <typename T>
    requires BoolLike<T>
constexpr std::size_t EstimateBoolSize() {
    return 5; // "false"
}

// Size for numbers
template <typename T>
    requires NumberLike<T>
constexpr std::size_t EstimateNumberSize() {
    using U = AnnotatedValue<T>;
    if constexpr (std::is_integral_v<U>) {
        return max_integer_digits<U>();
    } else {
        return max_float_size<U>();
    }
}

// Size for strings
template <typename T>
    requires SerializableStringLike<T>
constexpr std::size_t EstimateStringSize() {
    using U = AnnotatedValue<T>;
    
    std::size_t max_len = 0;
    
    // Only support fixed-size types (std::array<char, N> and similar)
    if constexpr (requires { std::tuple_size<U>::value; }) {
        constexpr std::size_t N = std::tuple_size<U>::value;
        // Assuming null-terminated string in array, actual length is N-1
        // But for safety, use full N for size calculation
        max_len = N;
    } else {
        static_assert(sizeof(T) == 0, "EstimateStringSize: only fixed-size string types (std::array<char, N>) are supported");
    }
    
    // Add quotes (2 bytes) + account for escaped characters
    // Worst case: every char is a control character escaped as \u00XX (6 bytes each)
    // Conservative estimate: 6 * max_len + 2 for quotes
    return 6 * max_len + 2;
}

// Size for arrays/vectors
template <typename T>
    requires SerializableArrayLike<T>
constexpr std::size_t EstimateArraySize() {
    using ElemT = typename array_read_cursor<AnnotatedValue<T>>::element_type;
    using U = AnnotatedValue<T>;
    
    std::size_t max_items = 0;
    
    // Only support fixed-size arrays
    if constexpr (requires { std::tuple_size<U>::value; }) {
        max_items = std::tuple_size<U>::value;
    } else {
        static_assert(sizeof(T) == 0, "EstimateArraySize: only fixed-size arrays (std::array<T, N>) are supported");
    }
    
    if (max_items == 0) {
        return 2; // "[]"
    }
    
    // Calculate: '[' + elements + commas + ']'
    std::size_t elem_size = EstimateMaxSizeImpl<ElemT>();
    std::size_t total = 1; // '['
    total += max_items * elem_size; // all elements
    total += (max_items - 1); // commas between elements
    total += 1; // ']'
    
    return total;
}

// Size for maps
template <typename T>
    requires SerializableMapLike<T>
constexpr std::size_t EstimateMapSize() {
    using FH = map_read_cursor<AnnotatedValue<T>>;
    using KeyT = typename FH::key_type;
    using ValT = AnnotatedValue<typename FH::mapped_type>;
    using U = AnnotatedValue<T>;
    
    // Get map size from tuple_size
    std::size_t max_properties = 0;
    if constexpr (requires { std::tuple_size<U>::value; }) {
        max_properties = std::tuple_size<U>::value;
    } else {
        static_assert(sizeof(T) == 0, "EstimateMapSize: only fixed-size map types are supported");
    }
    
    if (max_properties == 0) {
        return 2; // "{}"
    }
    
    // Calculate key size based on key type
    std::size_t key_size = 0;
    if constexpr (std::integral<KeyT>) {
        // Integer keys: convert to string, need quotes too
        key_size = max_integer_digits<KeyT>() + 2; // digits + quotes
    } else if constexpr (SerializableStringLike<KeyT>) {
        // String keys: must be fixed-size
        if constexpr (requires { std::tuple_size<KeyT>::value; }) {
            constexpr std::size_t N = std::tuple_size<KeyT>::value;
            // Worst case: every char escaped as \u00XX (6 bytes) + quotes
            key_size = 6 * N + 2;
        } else {
            static_assert(sizeof(T) == 0, "EstimateMapSize: map keys must be fixed-size arrays");
        }
    } else {
        static_assert(sizeof(T) == 0, "EstimateMapSize: map keys must be integral or string-like types");
    }
    
    // Calculate value size
    std::size_t val_size = EstimateMaxSizeImpl<ValT>();
    
    // Calculate: '{' + (key:value + comma) * N - last_comma + '}'
    std::size_t total = 1; // '{'
    total += max_properties * (key_size + 1 + val_size); // key + ':' + value
    total += (max_properties - 1); // commas between entries
    total += 1; // '}'
    
    return total;
}

// Size for objects (structs)
template <typename Type>
    requires ObjectLike<Type>
constexpr std::size_t EstimateObjectSize() {
    using T = AnnotatedValue<Type>;
    constexpr std::size_t N = introspection::structureElementsCount<T>;
    
    std::size_t total = 1; // '{'
    
    // Sum up all fields
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        auto calc_field = [&]<std::size_t Idx>() {
            using Field   = introspection::structureElementTypeByIndex<Idx, T>;
            using FieldOpts    = options::detail::annotation_meta_getter<Field>::options;
            
            // Check if field is excluded
            if constexpr (FieldOpts::template has_option<options::detail::exclude_tag>) {
                return 0zu;
            } else {
                // "fieldname":value
                // Calculate precise serialized key size at compile-time
                std::size_t serialized_key_size;
                if constexpr (FieldOpts::template has_option<options::detail::key_tag>) {
                    using KeyOpt = typename FieldOpts::template get_option<options::detail::key_tag>;
                    constexpr std::string_view field_name = KeyOpt::desc.toStringView();
                    serialized_key_size = CalculateSerializedStringSize(field_name);
                } else {
                    constexpr std::string_view field_name = introspection::structureElementNameByIndex<Idx, T>;
                    serialized_key_size = CalculateSerializedStringSize(field_name);
                }
                
                std::size_t field_size = 0;
                field_size += serialized_key_size; // Precise serialized key size (includes quotes)
                field_size += 1; // ':'
                field_size += EstimateMaxSizeImpl<Field>(); // value
                
                return field_size;
            }
        };
        
        std::size_t field_sizes[] = {calc_field.template operator()<I>()...};
        for (std::size_t size : field_sizes) {
            total += size;
        }
        
        // Add commas (one less than number of non-excluded fields)
        std::size_t non_zero_count = 0;
        for (std::size_t size : field_sizes) {
            if (size > 0) ++non_zero_count;
        }
        if (non_zero_count > 0) {
            total += (non_zero_count - 1); // commas
        }
        
    }(std::make_index_sequence<N>{});
    
    total += 1; // '}'
    return total;
}



// Main implementation
template <typename Type>
    requires SerializableValue<Type>
constexpr std::size_t EstimateMaxSizeImpl() {
    using T = AnnotatedValue<Type>;
    
    // Handle WireSink specially (it stores raw JSON)
    if constexpr (WireSinkLike<T>) {
        static_assert(sizeof(Type) == 0, "EstimateMaxSizeImpl: WireSink is unbounded in generic case");
        return 0;
    } else if constexpr (SerializeTransformerLike<T>) {
        return EstimateMaxSizeImpl<typename T::wire_type>();
    } else if constexpr (input_checks::is_specialization_of_v<T, std::optional>) {
        return EstimateMaxSizeImpl<typename T::value_type>();
    } else if constexpr (input_checks::is_specialization_of_v<T, std::unique_ptr>) {
        return EstimateMaxSizeImpl<typename T::element_type>();
    }
    // Handle primitives
    else if constexpr (BoolLike<T>) {
        return EstimateBoolSize<Type>();
    } else if constexpr (NumberLike<T>) {
        return EstimateNumberSize<Type>();
    } else if constexpr (SerializableStringLike<T>) {
        return EstimateStringSize<Type>();
    }
    // Handle arrays
    else if constexpr (SerializableArrayLike<T>) {
        return EstimateArraySize<Type>();
    }
    // Handle maps
    else if constexpr (SerializableMapLike<T>) {
        return EstimateMapSize<Type>();
    }
    // Handle objects (structs)
    else {
        return EstimateObjectSize<Type>();
    }
}

} // namespace detail

/// Estimate maximum serialized JSON size for a type at compile-time
/// @tparam T The type to estimate size for (must satisfy SerializableValue)
/// @return Maximum number of bytes needed to serialize this type to JSON
/// @note This is a conservative upper bound. Actual serialized size may be smaller.
template <typename T>
    requires SerializableValue<T>
constexpr std::size_t EstimateMaxSerializedSize() {
    return detail::EstimateMaxSizeImpl<T>();
}

} // namespace JsonFusion::size_estimator

