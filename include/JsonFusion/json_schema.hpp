#pragma once

#include "static_schema.hpp"
#include "writer_concept.hpp"
#include "options.hpp"
#include "struct_introspection.hpp"
#include "validators.hpp"
#include <type_traits>
#include <limits>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace JsonFusion::json_schema {

using namespace static_schema;
using namespace JsonFusion::validators::validators_detail;
namespace detail {

// Format unsigned integer to string (simplified from json.hpp format_decimal_integer)
// Returns pointer to one-past-last character written
constexpr char* format_unsigned_integer(std::size_t value, char* first, char* last) noexcept {
    // Generate digits in reverse order into the end of the buffer
    char* p = last;
    
    do {
        if (p == first) {
            // Not enough space; best-effort: just stop
            break;
        }
        unsigned digit = static_cast<unsigned>(value % 10u);
        value /= 10u;
        *--p = static_cast<char>('0' + digit);
    } while (value != 0);
    
    // Move result to start at 'first'
    std::size_t len = static_cast<std::size_t>(last - p);
    for (std::size_t i = 0; i < len; ++i) {
        first[i] = p[i];
    }
    
    return first + len;
}

// Helper to write a string key-value pair in an object
template<writer::WriterLike W>
constexpr bool write_property(W & writer, typename W::MapFrame& frame, const char* key, const char* value) {
    if (!writer.advance_after_value(frame)) return false;
    if (!writer.write_string(key, std::strlen(key), true)) return false;
    if (!writer.move_to_value(frame)) return false;
    return writer.write_string(value, std::strlen(value), true);
}

// Helper to write a numeric key-value pair
template<writer::WriterLike W, typename NumT>
constexpr bool write_property_number(W & writer, typename W::MapFrame& frame, const char* key, NumT value) {
    if (!writer.advance_after_value(frame)) return false;
    if (!writer.write_string(key, std::strlen(key), true)) return false;
    if (!writer.move_to_value(frame)) return false;
    return writer.template write_number<NumT>(value);
}

// Helper to write a boolean key-value pair
template<writer::WriterLike W>
constexpr bool write_property_bool(W & writer, typename W::MapFrame& frame, const char* key, bool value) {
    if (!writer.advance_after_value(frame)) return false;
    if (!writer.write_string(key, std::strlen(key), true)) return false;
    if (!writer.move_to_value(frame)) return false;
    return writer.write_bool(value);
}

// Forward declaration
template <typename Type, class... SeenTypes, writer::WriterLike W>
    requires ParsableValue<Type>
constexpr bool WriteSchemaImpl(W & writer);

// Write schema for boolean types
template <typename T, writer::WriterLike W>
    requires BoolLike<T>
constexpr bool WriteBoolSchema(W & writer) {
    typename W::MapFrame frame;
    std::size_t size = 1;
    if (!writer.write_map_begin(size, frame)) return false;
    if (!writer.write_string("type", 4, true)) return false;
    if (!writer.move_to_value(frame)) return false;
    if (!writer.write_string("boolean", 7, true)) return false;
    return writer.write_map_end(frame);
}

// Write schema for string types
template <typename T, writer::WriterLike W>
    requires StringLike<T>
constexpr bool WriteStringSchema(W & writer) {
    using Opts = typename options::detail::annotation_meta_getter<T>::options;

    
    typename W::MapFrame frame;

    if constexpr (Opts::template has_option<validators_options_tags::string_constant_tag>) { 
        if (!writer.write_map_begin(1, frame)) return false;
        if (!writer.write_string("const", 5, true)) return false;
        if (!writer.move_to_value(frame)) return false;
        using StringConstantOpt = typename Opts::template get_option<validators_options_tags::string_constant_tag>;
        if (!writer.write_string(StringConstantOpt::value.toStringView().data(), StringConstantOpt::value.toStringView().size(), true)) return false;
    } else if constexpr(Opts::template has_option<validators_options_tags::enum_values_tag>){
        if (!writer.write_map_begin(1, frame)) return false;
        if (!writer.write_string("enum", 4, true)) return false;
        if (!writer.move_to_value(frame)) return false;
        using EnumValuesOpt = typename Opts::template get_option<validators_options_tags::enum_values_tag>;
        typename W::ArrayFrame arr_frame;
        if (!writer.write_array_begin(EnumValuesOpt::valueCount, arr_frame)) return false;  
        for(std::size_t i = 0; i < EnumValuesOpt::valueCount; ++i) {
            if(i != 0) {
                if(!writer.advance_after_value(arr_frame)) return false;
            }
            if (!writer.write_string(EnumValuesOpt::sortedValues[i].name.data(), EnumValuesOpt::sortedValues[i].name.size(), true)) return false;
        }
        if (!writer.write_array_end(arr_frame)) return false;
    } else  {
        std::size_t prop_count = 1; // "type"
        if constexpr (Opts::template has_option<validators_options_tags::min_length_tag>) {
            prop_count += 1;
        }
        if constexpr (Opts::template has_option<validators_options_tags::max_length_tag>) {
            prop_count += 1;
        }

        if constexpr (Opts::template has_option<validators_options_tags::string_constant_tag>) {
            prop_count += 1;
        }
        
        if (!writer.write_map_begin(prop_count, frame)) return false;
        
        // Write type
        if (!writer.write_string("type", 4, true)) return false;
        if (!writer.move_to_value(frame)) return false;
        if (!writer.write_string("string", 6, true)) return false;


        if constexpr (Opts::template has_option<validators_options_tags::min_length_tag>) {
            if(!writer.advance_after_value(frame)) return false;
            if (!writer.write_string("minLength", 9, true)) return false;
            if (!writer.move_to_value(frame)) return false;
            using MinLengthOpt = typename Opts::template get_option<validators_options_tags::min_length_tag>;
            if (!writer.template write_number<std::size_t>(MinLengthOpt::value)) return false;
        }
        if constexpr (Opts::template has_option<validators_options_tags::max_length_tag>) {
            if(!writer.advance_after_value(frame)) return false;
            if (!writer.write_string("maxLength", 9, true)) return false;
            if (!writer.move_to_value(frame)) return false;
            using MaxLengthOpt = typename Opts::template get_option<validators_options_tags::max_length_tag>;
            if (!writer.template write_number<std::size_t>(MaxLengthOpt::value)) return false;
        }
    } 
    return writer.write_map_end(frame);
}

// Write schema for numeric types
template <typename T, writer::WriterLike W>
    requires NumberLike<T>
constexpr bool WriteNumberSchema(W & writer) {
    using Opts = typename options::detail::annotation_meta_getter<T>::options;

    using BaseT = AnnotatedValue<T>;
    
    typename W::MapFrame frame;

    if constexpr (!Opts::template has_option<validators_options_tags::constant_tag>) {
        std::size_t prop_count = 1; // "type"
        if constexpr (Opts::template has_option<validators_options_tags::range_tag>) {
            prop_count += 2;
        }
        if (!writer.write_map_begin(prop_count, frame)) return false;
        
        // Write type
        if (!writer.write_string("type", 4, true)) return false;
        if (!writer.move_to_value(frame)) return false;
        if constexpr (std::is_integral_v<BaseT>) {
            if (!writer.write_string("integer", 7, true)) return false;
        } else {
            if (!writer.write_string("number", 6, true)) return false;
        }
        if constexpr (Opts::template has_option<validators_options_tags::range_tag>) {
            if(!writer.advance_after_value(frame)) return false;
            if (!writer.write_string("minimum", 7, true)) return false;
            if (!writer.move_to_value(frame)) return false;
            using RangeOpt = typename Opts::template get_option<validators_options_tags::range_tag>;
            if (!writer.template write_number<BaseT>(RangeOpt::min)) return false;
        
            if(!writer.advance_after_value(frame)) return false;
            if (!writer.write_string("maximum", 7, true)) return false;
            if (!writer.move_to_value(frame)) return false;
            if (!writer.template write_number<BaseT>(RangeOpt::max)) return false;
        }
    } else {
        if (!writer.write_map_begin(1, frame)) return false;
        if (!writer.write_string("const", 5, true)) return false;
        if (!writer.move_to_value(frame)) return false;
        using ConstantOpt = typename Opts::template get_option<validators_options_tags::constant_tag>;
        if (!writer.template write_number<BaseT>(ConstantOpt::value)) return false;
    }
    
    return writer.write_map_end(frame);
}

// Write schema for null
template <writer::WriterLike W>
constexpr bool WriteNullSchema(W & writer) {
    typename W::MapFrame frame;
    std::size_t size = 1;
    if (!writer.write_map_begin(size, frame)) return false;
    if (!writer.write_string("type", 4, true)) return false;
    if (!writer.move_to_value(frame)) return false;
    if (!writer.write_string("null", 4, true)) return false;
    return writer.write_map_end(frame);
}

// Write schema for array types
template <typename T, class... SeenTypes, writer::WriterLike W>
    requires ParsableArrayLike<T>
constexpr bool WriteArraySchema(W & writer) {
    // TODO: Extract validators (min_items, max_items) from options pack
    using ElemT = AnnotatedValue<typename array_write_cursor<AnnotatedValue<T>>::element_type>;
    using Opts = typename options::detail::annotation_meta_getter<T>::options;

    typename W::MapFrame frame;
    std::size_t prop_count = 2; // "type" and "items"
    
    if constexpr (Opts::template has_option<validators_options_tags::min_items_tag>) {
        prop_count += 1;
    }
    if constexpr (Opts::template has_option<validators_options_tags::max_items_tag>) {
        prop_count += 1;
    }
    
    if (!writer.write_map_begin(prop_count, frame)) return false;
    
    // Write type
    if (!writer.write_string("type", 4, true)) return false;
    if (!writer.move_to_value(frame)) return false;
    if (!writer.write_string("array", 5, true)) return false;
    if constexpr (Opts::template has_option<validators_options_tags::min_items_tag>) {
        if(!writer.advance_after_value(frame)) return false;
        if (!writer.write_string("minItems", 8, true)) return false;
        if (!writer.move_to_value(frame)) return false;
        using MinItemsOpt = typename Opts::template get_option<validators_options_tags::min_items_tag>;
        if (!writer.template write_number<std::size_t>(MinItemsOpt::value)) return false;
    }
    if constexpr (Opts::template has_option<validators_options_tags::max_items_tag>) {
        if(!writer.advance_after_value(frame)) return false;
        if (!writer.write_string("maxItems", 8, true)) return false;
        if (!writer.move_to_value(frame)) return false;
        using MaxItemsOpt = typename Opts::template get_option<validators_options_tags::max_items_tag>;
        if (!writer.template write_number<std::size_t>(MaxItemsOpt::value)) return false;
    }
    // Write items schema
    if (!writer.advance_after_value(frame)) return false;
    if (!writer.write_string("items", 5, true)) return false;
    if (!writer.move_to_value(frame)) return false;
    // Add the unwrapped array type to SeenTypes for cycle detection
    using UnwrappedArrayType = AnnotatedValue<T>;
    if (!WriteSchemaImpl<ElemT, UnwrappedArrayType, SeenTypes...>(writer)) return false;
    
    return writer.write_map_end(frame);
}

// Write schema for map types (additionalProperties pattern)
template <typename T, class... SeenTypes, writer::WriterLike W>
    requires ParsableMapLike<T>
constexpr bool WriteMapSchema(W & writer) {
    using ElemT = AnnotatedValue<typename map_write_cursor<AnnotatedValue<T>>::mapped_type>;
    using Opts = typename options::detail::annotation_meta_getter<T>::options;
    
    // Static assertions for invalid combinations
    static_assert(!(Opts::template has_option<validators_options_tags::allowed_keys_tag> && 
                    Opts::template has_option<validators_options_tags::forbidden_keys_tag>),
                  "allowed_keys and forbidden_keys are mutually exclusive (whitelist vs blacklist)");
    
    // Check if required_keys is subset of allowed_keys
    if constexpr (Opts::template has_option<validators_options_tags::required_keys_tag> && 
                  Opts::template has_option<validators_options_tags::allowed_keys_tag>) {
        using RequiredKeysOpt = typename Opts::template get_option<validators_options_tags::required_keys_tag>;
        using AllowedKeysOpt = typename Opts::template get_option<validators_options_tags::allowed_keys_tag>;
        
        // Runtime check at compile time
        []() consteval {
            for (const auto& req_key : RequiredKeysOpt::sortedKeys) {
                bool found = false;
                for (const auto& allowed_key : AllowedKeysOpt::sortedKeys) {
                    if (req_key.name == allowed_key.name) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    throw "required_keys must be subset of allowed_keys";
                }
            }
        }();
    }
    
    constexpr bool has_allowed_keys = Opts::template has_option<validators_options_tags::allowed_keys_tag>;
    constexpr bool has_required_keys = Opts::template has_option<validators_options_tags::required_keys_tag>;
    constexpr bool has_forbidden_keys = Opts::template has_option<validators_options_tags::forbidden_keys_tag>;
    constexpr bool has_key_length = Opts::template has_option<validators_options_tags::min_key_length_tag> || 
                                    Opts::template has_option<validators_options_tags::max_key_length_tag>;
    constexpr bool needs_propertyNames = has_key_length || has_forbidden_keys;
    constexpr bool needs_properties = has_allowed_keys || has_required_keys;
    
    typename W::MapFrame frame;
    std::size_t prop_count = 2; // "type" and "additionalProperties"
    
    if constexpr (Opts::template has_option<validators_options_tags::min_properties_tag>) ++prop_count;
    if constexpr (Opts::template has_option<validators_options_tags::max_properties_tag>) ++prop_count;
    if constexpr (needs_propertyNames) ++prop_count;
    if constexpr (needs_properties) ++prop_count; // "properties"
    if constexpr (has_required_keys) ++prop_count; // "required"
    
    if (!writer.write_map_begin(prop_count, frame)) return false;
    
    // Write type
    if (!writer.write_string("type", 4, true)) return false;
    if (!writer.move_to_value(frame)) return false;
    if (!writer.write_string("object", 6, true)) return false;
    
    // Write minProperties
    if constexpr (Opts::template has_option<validators_options_tags::min_properties_tag>) {
        if (!writer.advance_after_value(frame)) return false;
        if (!writer.write_string("minProperties", 13, true)) return false;
        if (!writer.move_to_value(frame)) return false;
        using MinPropertiesOpt = typename Opts::template get_option<validators_options_tags::min_properties_tag>;
        if (!writer.template write_number<std::size_t>(MinPropertiesOpt::value)) return false;
    }
    
    // Write maxProperties
    if constexpr (Opts::template has_option<validators_options_tags::max_properties_tag>) {
        if (!writer.advance_after_value(frame)) return false;
        if (!writer.write_string("maxProperties", 13, true)) return false;
        if (!writer.move_to_value(frame)) return false;
        using MaxPropertiesOpt = typename Opts::template get_option<validators_options_tags::max_properties_tag>;
        if (!writer.template write_number<std::size_t>(MaxPropertiesOpt::value)) return false;
    }
    
    // Write propertyNames (for key length constraints and/or forbidden keys)
    if constexpr (needs_propertyNames) {
        if (!writer.advance_after_value(frame)) return false;
        if (!writer.write_string("propertyNames", 13, true)) return false;
        if (!writer.move_to_value(frame)) return false;
        
        constexpr std::size_t constraint_count = (has_key_length ? 1 : 0) + (has_forbidden_keys ? 1 : 0);
        
        if constexpr (constraint_count == 1) {
            // Single constraint - write directly
            if constexpr (has_key_length) {
                typename W::MapFrame key_frame;
                std::size_t key_prop_count = 0;
                if constexpr (Opts::template has_option<validators_options_tags::min_key_length_tag>) ++key_prop_count;
                if constexpr (Opts::template has_option<validators_options_tags::max_key_length_tag>) ++key_prop_count;
                
                if (!writer.write_map_begin(key_prop_count, key_frame)) return false;
                bool first = true;
                
                if constexpr (Opts::template has_option<validators_options_tags::min_key_length_tag>) {
                    if (!writer.write_string("minLength", 9, true)) return false;
                    if (!writer.move_to_value(key_frame)) return false;
                    using MinKeyLengthOpt = typename Opts::template get_option<validators_options_tags::min_key_length_tag>;
                    if (!writer.template write_number<std::size_t>(MinKeyLengthOpt::value)) return false;
                    first = false;
                }
                
                if constexpr (Opts::template has_option<validators_options_tags::max_key_length_tag>) {
                    if (!first && !writer.advance_after_value(key_frame)) return false;
                    if (!writer.write_string("maxLength", 9, true)) return false;
                    if (!writer.move_to_value(key_frame)) return false;
                    using MaxKeyLengthOpt = typename Opts::template get_option<validators_options_tags::max_key_length_tag>;
                    if (!writer.template write_number<std::size_t>(MaxKeyLengthOpt::value)) return false;
                }
                
                if (!writer.write_map_end(key_frame)) return false;
            } else if constexpr (has_forbidden_keys) {
                // {"not": {"enum": [...]}}
                using ForbiddenKeysOpt = typename Opts::template get_option<validators_options_tags::forbidden_keys_tag>;
                
                typename W::MapFrame not_frame;
                if (!writer.write_map_begin(1, not_frame)) return false;
                if (!writer.write_string("not", 3, true)) return false;
                if (!writer.move_to_value(not_frame)) return false;
                
                typename W::MapFrame enum_frame;
                if (!writer.write_map_begin(1, enum_frame)) return false;
                if (!writer.write_string("enum", 4, true)) return false;
                if (!writer.move_to_value(enum_frame)) return false;
                
                typename W::ArrayFrame arr_frame;
                if (!writer.write_array_begin(ForbiddenKeysOpt::keyCount, arr_frame)) return false;
                for (std::size_t i = 0; i < ForbiddenKeysOpt::keyCount; ++i) {
                    if (i > 0 && !writer.advance_after_value(arr_frame)) return false;
                    if (!writer.write_string(ForbiddenKeysOpt::sortedKeys[i].name.data(), 
                                            ForbiddenKeysOpt::sortedKeys[i].name.size(), true)) return false;
                }
                if (!writer.write_array_end(arr_frame)) return false;
                if (!writer.write_map_end(enum_frame)) return false;
                if (!writer.write_map_end(not_frame)) return false;
            }
        } else {
            // Multiple constraints - use allOf
            typename W::MapFrame allof_frame;
            if (!writer.write_map_begin(1, allof_frame)) return false;
            if (!writer.write_string("allOf", 5, true)) return false;
            if (!writer.move_to_value(allof_frame)) return false;
            
            typename W::ArrayFrame arr_frame;
            if (!writer.write_array_begin(constraint_count, arr_frame)) return false;
            
            bool first_constraint = true;
            
            // Add key length constraint
            if constexpr (has_key_length) {
                typename W::MapFrame key_frame;
                std::size_t key_prop_count = 0;
                if constexpr (Opts::template has_option<validators_options_tags::min_key_length_tag>) ++key_prop_count;
                if constexpr (Opts::template has_option<validators_options_tags::max_key_length_tag>) ++key_prop_count;
                
                if (!writer.write_map_begin(key_prop_count, key_frame)) return false;
                bool first = true;
                
                if constexpr (Opts::template has_option<validators_options_tags::min_key_length_tag>) {
                    if (!writer.write_string("minLength", 9, true)) return false;
                    if (!writer.move_to_value(key_frame)) return false;
                    using MinKeyLengthOpt = typename Opts::template get_option<validators_options_tags::min_key_length_tag>;
                    if (!writer.template write_number<std::size_t>(MinKeyLengthOpt::value)) return false;
                    first = false;
                }
                
                if constexpr (Opts::template has_option<validators_options_tags::max_key_length_tag>) {
                    if (!first && !writer.advance_after_value(key_frame)) return false;
                    if (!writer.write_string("maxLength", 9, true)) return false;
                    if (!writer.move_to_value(key_frame)) return false;
                    using MaxKeyLengthOpt = typename Opts::template get_option<validators_options_tags::max_key_length_tag>;
                    if (!writer.template write_number<std::size_t>(MaxKeyLengthOpt::value)) return false;
                }
                
                if (!writer.write_map_end(key_frame)) return false;
                first_constraint = false;
            }
            
            // Add forbidden keys constraint
            if constexpr (has_forbidden_keys) {
                if (!first_constraint && !writer.advance_after_value(arr_frame)) return false;
                
                using ForbiddenKeysOpt = typename Opts::template get_option<validators_options_tags::forbidden_keys_tag>;
                
                typename W::MapFrame not_frame;
                if (!writer.write_map_begin(1, not_frame)) return false;
                if (!writer.write_string("not", 3, true)) return false;
                if (!writer.move_to_value(not_frame)) return false;
                
                typename W::MapFrame enum_frame;
                if (!writer.write_map_begin(1, enum_frame)) return false;
                if (!writer.write_string("enum", 4, true)) return false;
                if (!writer.move_to_value(enum_frame)) return false;
                
                typename W::ArrayFrame enum_arr_frame;
                if (!writer.write_array_begin(ForbiddenKeysOpt::keyCount, enum_arr_frame)) return false;
                for (std::size_t i = 0; i < ForbiddenKeysOpt::keyCount; ++i) {
                    if (i > 0 && !writer.advance_after_value(enum_arr_frame)) return false;
                    if (!writer.write_string(ForbiddenKeysOpt::sortedKeys[i].name.data(), 
                                            ForbiddenKeysOpt::sortedKeys[i].name.size(), true)) return false;
                }
                if (!writer.write_array_end(enum_arr_frame)) return false;
                if (!writer.write_map_end(enum_frame)) return false;
                if (!writer.write_map_end(not_frame)) return false;
            }
            
            if (!writer.write_array_end(arr_frame)) return false;
            if (!writer.write_map_end(allof_frame)) return false;
        }
    }
    
    // Write properties (for allowed_keys or required_keys)
    if constexpr (needs_properties) {
        if (!writer.advance_after_value(frame)) return false;
        if (!writer.write_string("properties", 10, true)) return false;
        if (!writer.move_to_value(frame)) return false;
        
        typename W::MapFrame props_frame;
        std::size_t keys_count = 0;
        
        if constexpr (has_allowed_keys) {
            using AllowedKeysOpt = typename Opts::template get_option<validators_options_tags::allowed_keys_tag>;
            keys_count = AllowedKeysOpt::keyCount;
        } else if constexpr (has_required_keys) {
            using RequiredKeysOpt = typename Opts::template get_option<validators_options_tags::required_keys_tag>;
            keys_count = RequiredKeysOpt::keyCount;
        }
        
        if (!writer.write_map_begin(keys_count, props_frame)) return false;
        
        // Write each key's schema
        if constexpr (has_allowed_keys) {
            using AllowedKeysOpt = typename Opts::template get_option<validators_options_tags::allowed_keys_tag>;
            for (std::size_t i = 0; i < AllowedKeysOpt::keyCount; ++i) {
                if (i > 0 && !writer.advance_after_value(props_frame)) return false;
                if (!writer.write_string(AllowedKeysOpt::sortedKeys[i].name.data(),
                                        AllowedKeysOpt::sortedKeys[i].name.size(), true)) return false;
                if (!writer.move_to_value(props_frame)) return false;
                // Add the unwrapped map type to SeenTypes for cycle detection
                using UnwrappedMapType = AnnotatedValue<T>;
                if (!WriteSchemaImpl<ElemT, UnwrappedMapType, SeenTypes...>(writer)) return false;
            }
        } else if constexpr (has_required_keys) {
            using RequiredKeysOpt = typename Opts::template get_option<validators_options_tags::required_keys_tag>;
            for (std::size_t i = 0; i < RequiredKeysOpt::keyCount; ++i) {
                if (i > 0 && !writer.advance_after_value(props_frame)) return false;
                if (!writer.write_string(RequiredKeysOpt::sortedKeys[i].name.data(),
                                        RequiredKeysOpt::sortedKeys[i].name.size(), true)) return false;
                if (!writer.move_to_value(props_frame)) return false;
                // Add the unwrapped map type to SeenTypes for cycle detection
                using UnwrappedMapType = AnnotatedValue<T>;
                if (!WriteSchemaImpl<ElemT, UnwrappedMapType, SeenTypes...>(writer)) return false;
            }
        }
        
        if (!writer.write_map_end(props_frame)) return false;
    }
    
    // Write required array (only for required_keys)
    if constexpr (has_required_keys) {
        using RequiredKeysOpt = typename Opts::template get_option<validators_options_tags::required_keys_tag>;
        
        if (!writer.advance_after_value(frame)) return false;
        if (!writer.write_string("required", 8, true)) return false;
        if (!writer.move_to_value(frame)) return false;
        
        typename W::ArrayFrame req_frame;
        if (!writer.write_array_begin(RequiredKeysOpt::keyCount, req_frame)) return false;
        for (std::size_t i = 0; i < RequiredKeysOpt::keyCount; ++i) {
            if (i > 0 && !writer.advance_after_value(req_frame)) return false;
            if (!writer.write_string(RequiredKeysOpt::sortedKeys[i].name.data(),
                                    RequiredKeysOpt::sortedKeys[i].name.size(), true)) return false;
        }
        if (!writer.write_array_end(req_frame)) return false;
    }
    
    // Write additionalProperties
    if (!writer.advance_after_value(frame)) return false;
    if (!writer.write_string("additionalProperties", 20, true)) return false;
    if (!writer.move_to_value(frame)) return false;
    
    if constexpr (has_allowed_keys) {
        // Closed set - no additional properties allowed
        if (!writer.write_bool(false)) return false;
    } else {
        // Open set - additional properties have the value schema
        // Add the unwrapped map type to SeenTypes for cycle detection
        using UnwrappedMapType = AnnotatedValue<T>;
        if (!WriteSchemaImpl<ElemT, UnwrappedMapType, SeenTypes...>(writer)) return false;
    }
    
    return writer.write_map_end(frame);
}

// Write schema for object types (structs) as array (tuple)
template <typename T, class... SeenTypes, writer::WriterLike W>
    requires ObjectLike<T>
constexpr bool WriteObjectSchemaAsArray(W & writer) {
    using UnderlyingT = AnnotatedValue<T>;
    constexpr std::size_t struct_elements_count = introspection::structureElementsCount<UnderlyingT>;
    
    // Count non-excluded fields
    std::size_t field_count = 0;
    if constexpr (struct_elements_count > 0) {
        [&]<std::size_t... I>(std::index_sequence<I...>) constexpr {
            ((([&] constexpr {
                using Field = introspection::structureElementTypeByIndex<I, UnderlyingT>;
                using FieldOpts = typename options::detail::annotation_meta_getter<Field>::options;
                if constexpr (!FieldOpts::template has_option<options::detail::exclude_tag>) {
                    ++field_count;
                }
            })(), ...));
        }(std::make_index_sequence<struct_elements_count>{});
    }
    
    typename W::MapFrame frame;
    std::size_t prop_count = 4; // "type", "prefixItems", "minItems", "maxItems"
    
    if (!writer.write_map_begin(prop_count, frame)) return false;
    
    // Write type
    if (!writer.write_string("type", 4, true)) return false;
    if (!writer.move_to_value(frame)) return false;
    if (!writer.write_string("array", 5, true)) return false;
    
    // Write prefixItems (tuple schema)
    if (!writer.advance_after_value(frame)) return false;
    if (!writer.write_string("prefixItems", 11, true)) return false;
    if (!writer.move_to_value(frame)) return false;
    
    typename W::ArrayFrame items_frame;
    if (!writer.write_array_begin(field_count, items_frame)) return false;
    
    // Write each field's schema in order
    if constexpr (struct_elements_count > 0) {
        bool first = true;
        [&]<std::size_t... I>(std::index_sequence<I...>) constexpr {
            ((([&] constexpr {
                using Field = introspection::structureElementTypeByIndex<I, UnderlyingT>;
                using FieldOpts = typename options::detail::annotation_meta_getter<Field>::options;
                
                if constexpr (!FieldOpts::template has_option<options::detail::exclude_tag>) {
                    if (!first) {
                        if (!writer.advance_after_value(items_frame)) return;
                    }
                    first = false;
                    
                    if (!WriteSchemaImpl<Field, AnnotatedValue<T>, SeenTypes...>(writer)) return;
                }
            })(), ...));
        }(std::make_index_sequence<struct_elements_count>{});
    }
    
    if (!writer.write_array_end(items_frame)) return false;
    
    // Write minItems (exact count required)
    if (!writer.advance_after_value(frame)) return false;
    if (!writer.write_string("minItems", 8, true)) return false;
    if (!writer.move_to_value(frame)) return false;
    if (!writer.template write_number<std::size_t>(field_count)) return false;
    
    // Write maxItems (exact count required)
    if (!writer.advance_after_value(frame)) return false;
    if (!writer.write_string("maxItems", 8, true)) return false;
    if (!writer.move_to_value(frame)) return false;
    if (!writer.template write_number<std::size_t>(field_count)) return false;
    
    return writer.write_map_end(frame);
}

// Write schema for object types (structs)
template <typename T, class... SeenTypes, writer::WriterLike W>
    requires ObjectLike<T>
constexpr bool WriteObjectSchema(W & writer) {
    using Opts = typename options::detail::annotation_meta_getter<T>::options;
    
    // Check if as_array option is present
    if constexpr (Opts::template has_option<options::detail::as_array_tag>) {
        return WriteObjectSchemaAsArray<T, SeenTypes...>(writer);
    }
    
    using UnderlyingT = AnnotatedValue<T>;
    constexpr std::size_t struct_elements_count = introspection::structureElementsCount<UnderlyingT>;
    
    // Check if indexes_as_keys should be enabled:
    // 1. Explicitly set via indexes_as_keys option, OR
    // 2. At least one field has int_key<N>
    constexpr bool has_indexes_as_keys_option = Opts::template has_option<options::detail::indexes_as_keys_tag>;
    constexpr bool has_any_int_key = []() consteval {
        if constexpr (struct_elements_count > 0) {
            bool found = false;
            [&]<std::size_t... I>(std::index_sequence<I...>) {
                ((([&] {
                    using Field = introspection::structureElementTypeByIndex<I, UnderlyingT>;
                    using FieldOpts = typename options::detail::annotation_meta_getter<Field>::options;
                    if constexpr (FieldOpts::template has_option<options::detail::numeric_key_tag>) {
                        found = true;
                    }
                })(), ...));
            }(std::make_index_sequence<struct_elements_count>{});
            return found;
        }
        return false;
    }();
    constexpr bool uses_indexes_as_keys = has_indexes_as_keys_option || has_any_int_key;
    
    typename W::MapFrame obj_frame;
    std::size_t obj_prop_count = 2; // "type" and "properties"
    
    // Check if we have forbidden validator
    constexpr bool has_forbidden = Opts::template has_option<validators_options_tags::forbidden_tag>;
    
    // Check if we need "required" array based on validators
    bool has_required_array = false;
    std::size_t required_count = 0;
    
    if constexpr (Opts::template has_option<validators_options_tags::required_tag>) {
        // Only fields listed in required<...> validator
        using RequiredOpt = typename Opts::template get_option<validators_options_tags::required_tag>;
        required_count = RequiredOpt::values.size();
        has_required_array = (required_count > 0);
    } else if constexpr (Opts::template has_option<validators_options_tags::not_required_tag>) {
        // All fields EXCEPT those listed in not_required<...> validator
        using NotRequiredOpt = typename Opts::template get_option<validators_options_tags::not_required_tag>;
        
        if constexpr (struct_elements_count > 0) {
            std::size_t current_index = 0;  // Like C++ enum: tracks the "next" index
            [&]<std::size_t... I>(std::index_sequence<I...>) constexpr {
                ((([&] constexpr {
                    using Field = introspection::structureElementTypeByIndex<I, UnderlyingT>;
                    using FieldOpts = typename options::detail::annotation_meta_getter<Field>::options;
                    if constexpr (!FieldOpts::template has_option<options::detail::exclude_tag>) {
                        // Get field name/index
                        char index_str_buf[32];
                        std::string_view field_name;
                        
                        if constexpr (uses_indexes_as_keys) {
                            // Calculate numeric index following C++ enum semantics
                            std::size_t field_index_num;
                            if constexpr (FieldOpts::template has_option<options::detail::numeric_key_tag>) {
                                using IntKeyOpt = typename FieldOpts::template get_option<options::detail::numeric_key_tag>;
                                field_index_num = IntKeyOpt::NumericKey;
                                current_index = field_index_num + 1;  // Next field will be this + 1
                            } else {
                                field_index_num = current_index;
                                current_index++;  // Auto-increment for next field
                            }
                            char* end = format_unsigned_integer(field_index_num, index_str_buf, index_str_buf + sizeof(index_str_buf));
                            field_name = std::string_view(index_str_buf, end - index_str_buf);
                        } else {
                            if constexpr (FieldOpts::template has_option<options::detail::key_tag>) {
                                using KeyOpt = typename FieldOpts::template get_option<options::detail::key_tag>;
                                field_name = KeyOpt::desc.toStringView();
                            } else {
                                field_name = introspection::structureElementNameByIndex<I, UnderlyingT>;
                            }
                        }
                        
                        // Check if field is NOT in the not_required list
                        bool is_not_required = false;
                        for (const auto& not_req_name : NotRequiredOpt::values) {
                            if (field_name == not_req_name) {
                                is_not_required = true;
                                break;
                            }
                        }
                        if (!is_not_required) {
                            ++required_count;
                        }
                    }
                })(), ...));
            }(std::make_index_sequence<struct_elements_count>{});
        }
        has_required_array = (required_count > 0);
    }
    // else: no validator means all fields optional (no "required" array)
    
    if (has_required_array) ++obj_prop_count;
    if (has_forbidden) ++obj_prop_count; // Add propertyNames for forbidden fields
    if constexpr (!Opts::template has_option<options::detail::allow_excess_fields_tag>) {
        ++obj_prop_count;
    }
    
    if (!writer.write_map_begin(obj_prop_count, obj_frame)) return false;
    
    if constexpr (!Opts::template has_option<options::detail::allow_excess_fields_tag>) {
        if (!writer.write_string("additionalProperties", 20, true)) return false;
        if (!writer.move_to_value(obj_frame)) return false;
        if (!writer.write_bool(false)) return false;
        if (!writer.advance_after_value(obj_frame)) return false;
    }
    // Write type
    if (!writer.write_string("type", 4, true)) return false;
    if (!writer.move_to_value(obj_frame)) return false;
    if (!writer.write_string("object", 6, true)) return false;
    
    // Write properties
    if (!writer.advance_after_value(obj_frame)) return false;
    if (!writer.write_string("properties", 10, true)) return false;
    if (!writer.move_to_value(obj_frame)) return false;
    
    typename W::MapFrame props_frame;
    std::size_t props_count = 0;
    
    // Count non-excluded fields
    if constexpr (struct_elements_count > 0) {
        [&]<std::size_t... I>(std::index_sequence<I...>) constexpr {
            ((([&] constexpr {
                using Field = introspection::structureElementTypeByIndex<I, UnderlyingT>;
                using FieldOpts = typename options::detail::annotation_meta_getter<Field>::options;
                if constexpr (!FieldOpts::template has_option<options::detail::exclude_tag>) {
                    ++props_count;
                }
            })(), ...));
        }(std::make_index_sequence<struct_elements_count>{});
    }
    
    if (!writer.write_map_begin(props_count, props_frame)) return false;
    
    // Write each field's schema
    if constexpr (struct_elements_count > 0) {
        bool first = true;
        std::size_t current_index = 0;  // Like C++ enum: tracks the "next" index
        [&]<std::size_t... I>(std::index_sequence<I...>) constexpr {
            ((([&] constexpr {
                using Field = introspection::structureElementTypeByIndex<I, UnderlyingT>;
                using FieldOpts = typename options::detail::annotation_meta_getter<Field>::options;
                
                if constexpr (!FieldOpts::template has_option<options::detail::exclude_tag>) {
                    using FieldT = AnnotatedValue<Field>;
                    
                    if (!first) {
                        if (!writer.advance_after_value(props_frame)) return;
                    }
                    first = false;
                    
                    // Get field name or index
                    char index_str_buf[32];
                    std::string_view field_name;
                    
                    if constexpr (uses_indexes_as_keys) {
                        // Calculate numeric index following C++ enum semantics
                        std::size_t field_index_num;
                        if constexpr (FieldOpts::template has_option<options::detail::numeric_key_tag>) {
                            using IntKeyOpt = typename FieldOpts::template get_option<options::detail::numeric_key_tag>;
                            field_index_num = IntKeyOpt::NumericKey;
                            current_index = field_index_num + 1;  // Next field will be this + 1
                        } else {
                            field_index_num = current_index;
                            current_index++;  // Auto-increment for next field
                        }
                        char* end = format_unsigned_integer(field_index_num, index_str_buf, index_str_buf + sizeof(index_str_buf));
                        field_name = std::string_view(index_str_buf, end - index_str_buf);
                    } else {
                        if constexpr (FieldOpts::template has_option<options::detail::key_tag>) {
                            using KeyOpt = typename FieldOpts::template get_option<options::detail::key_tag>;
                            field_name = KeyOpt::desc.toStringView();
                        } else {
                            field_name = introspection::structureElementNameByIndex<I, UnderlyingT>;
                        }
                    }
                    
                    if (!writer.write_string(field_name.data(), field_name.size(), true)) return;
                    if (!writer.move_to_value(props_frame)) return;
                    if (!WriteSchemaImpl<Field, AnnotatedValue<T>, SeenTypes...>(writer)) return;
                }
            })(), ...));
        }(std::make_index_sequence<struct_elements_count>{});
    }
    
    if (!writer.write_map_end(props_frame)) return false;
    
    // Write required array if needed
    if (has_required_array && required_count > 0) {
        if (!writer.advance_after_value(obj_frame)) return false;
        if (!writer.write_string("required", 8, true)) return false;
        if (!writer.move_to_value(obj_frame)) return false;
        
        typename W::ArrayFrame req_frame;
        if (!writer.write_array_begin(required_count, req_frame)) return false;
        
        if constexpr (Opts::template has_option<validators_options_tags::required_tag>) {
            // Write only fields listed in required<...> validator
            using RequiredOpt = typename Opts::template get_option<validators_options_tags::required_tag>;
            bool first = true;
            for (const auto& req_name : RequiredOpt::values) {
                if (!first) {
                    if (!writer.advance_after_value(req_frame)) return false;
                }
                first = false;
                if (!writer.write_string(req_name.data(), req_name.size(), true)) return false;
            }
        } else if constexpr (Opts::template has_option<validators_options_tags::not_required_tag>) {
            // Write all fields EXCEPT those listed in not_required<...> validator
            using NotRequiredOpt = typename Opts::template get_option<validators_options_tags::not_required_tag>;
            
            bool first = true;
            std::size_t current_index = 0;  // Like C++ enum: tracks the "next" index
            if constexpr (struct_elements_count > 0) {
                [&]<std::size_t... I>(std::index_sequence<I...>) constexpr {
                    ((([&] constexpr {
                        using Field = introspection::structureElementTypeByIndex<I, UnderlyingT>;
                        using FieldOpts = typename options::detail::annotation_meta_getter<Field>::options;
                        
                        if constexpr (!FieldOpts::template has_option<options::detail::exclude_tag>) {
                            // Get field name or index
                            char index_str_buf[32];
                            std::string_view field_name;
                            
                            if constexpr (uses_indexes_as_keys) {
                                // Calculate numeric index following C++ enum semantics
                                std::size_t field_index_num;
                                if constexpr (FieldOpts::template has_option<options::detail::numeric_key_tag>) {
                                    using IntKeyOpt = typename FieldOpts::template get_option<options::detail::numeric_key_tag>;
                                    field_index_num = IntKeyOpt::NumericKey;
                                    current_index = field_index_num + 1;  // Next field will be this + 1
                                } else {
                                    field_index_num = current_index;
                                    current_index++;  // Auto-increment for next field
                                }
                                char* end = format_unsigned_integer(field_index_num, index_str_buf, index_str_buf + sizeof(index_str_buf));
                                field_name = std::string_view(index_str_buf, end - index_str_buf);
                            } else {
                                if constexpr (FieldOpts::template has_option<options::detail::key_tag>) {
                                    using KeyOpt = typename FieldOpts::template get_option<options::detail::key_tag>;
                                    field_name = KeyOpt::desc.toStringView();
                                } else {
                                    field_name = introspection::structureElementNameByIndex<I, UnderlyingT>;
                                }
                            }
                            
                            // Check if field is NOT in the not_required list
                            bool is_not_required = false;
                            for (const auto& not_req_name : NotRequiredOpt::values) {
                                if (field_name == not_req_name) {
                                    is_not_required = true;
                                    break;
                                }
                            }
                            
                            if (!is_not_required) {
                                if (!first) {
                                    if (!writer.advance_after_value(req_frame)) return;
                                }
                                first = false;
                                if (!writer.write_string(field_name.data(), field_name.size(), true)) return;
                            }
                        }
                    })(), ...));
                }(std::make_index_sequence<struct_elements_count>{});
            }
        }
        
        if (!writer.write_array_end(req_frame)) return false;
    }
    
    // Write propertyNames for forbidden fields
    if constexpr (has_forbidden) {
        using ForbiddenOpt = typename Opts::template get_option<validators_options_tags::forbidden_tag>;
        
        if (!writer.advance_after_value(obj_frame)) return false;
        if (!writer.write_string("propertyNames", 13, true)) return false;
        if (!writer.move_to_value(obj_frame)) return false;
        
        // propertyNames: { "not": { "enum": [...] } }
        typename W::MapFrame prop_names_frame;
        if (!writer.write_map_begin(1, prop_names_frame)) return false;
        if (!writer.write_string("not", 3, true)) return false;
        if (!writer.move_to_value(prop_names_frame)) return false;
        
        typename W::MapFrame not_frame;
        if (!writer.write_map_begin(1, not_frame)) return false;
        if (!writer.write_string("enum", 4, true)) return false;
        if (!writer.move_to_value(not_frame)) return false;
        
        // Write array of forbidden field names
        typename W::ArrayFrame forbidden_frame;
        std::size_t forbidden_count = ForbiddenOpt::values.size();
        if (!writer.write_array_begin(forbidden_count, forbidden_frame)) return false;
        
        bool first = true;
        for (const auto& forbidden_name : ForbiddenOpt::values) {
            if (!first) {
                if (!writer.advance_after_value(forbidden_frame)) return false;
            }
            first = false;
            if (!writer.write_string(forbidden_name.data(), forbidden_name.size(), true)) return false;
        }
        
        if (!writer.write_array_end(forbidden_frame)) return false;
        if (!writer.write_map_end(not_frame)) return false;
        if (!writer.write_map_end(prop_names_frame)) return false;
    }
    
    return writer.write_map_end(obj_frame);
}

// Helper to check if a type appears in a pack
template<typename T, typename... Types>
constexpr bool type_in_pack_v = (std::is_same_v<T, Types> || ...);

// Get the last type in a parameter pack (the root schema type)
template<typename... Types>
struct last_type;

template<typename T>
struct last_type<T> { using type = T; };

template<typename T, typename... Rest>
struct last_type<T, Rest...> { using type = typename last_type<Rest...>::type; };

template<typename... Types>
using last_type_t = typename last_type<Types...>::type;

// Main recursive schema writer with cycle detection
template <typename Type, class... SeenTypes, writer::WriterLike W>
    requires ParsableValue<Type>
constexpr bool WriteSchemaImpl(W & writer) {
    using T = AnnotatedValue<Type>;
    
    // Check for cycles
    if constexpr (type_in_pack_v<T, SeenTypes...>) {
        // Recursive type detected
        // Check if this references the root schema (last type in SeenTypes)
        constexpr bool is_root_reference = []() consteval {
            if constexpr (sizeof...(SeenTypes) == 0) {
                return false; // No SeenTypes - shouldn't happen if we're in this branch
            } else {
                using RootType = last_type_t<SeenTypes...>;
                return std::is_same_v<T, RootType>;
            }
        }();
        
        if constexpr (is_root_reference) {
            // This is a self-reference to the root schema - emit {"$ref": "#"}
            typename W::MapFrame frame;
            std::size_t size = 1;
            if (!writer.write_map_begin(size, frame)) return false;
            if (!writer.write_string("$ref", 4, true)) return false;
            if (!writer.move_to_value(frame)) return false;
            if (!writer.write_string("#", 1, true)) return false;
            return writer.write_map_end(frame);
        } else {
            // This is a nested or mutual recursion - not supported
            static_assert(is_root_reference, 
                "JsonFusion: Recursive schema detected that does not reference the root schema. "
                "Only self-referencing types at the root level are supported (e.g., struct Tree { vector<Tree> children; }). "
                "Mutual recursion (A->B->A) and nested recursive types are not supported.");
            return false; // Unreachable, but needed for compilation
        }
    }
    else {
    // NO CYCLE - continue with normal schema generation
    
    // Check options on the original Type (before unwrapping annotations)
    using OptsOnType = typename options::detail::annotation_meta_getter<Type>::options;
    
    // Handle wire_sink - accepts any JSON value (empty schema)
    if constexpr (OptsOnType::template has_option<options::detail::wire_sink_tag>) {
        // Empty schema {} allows any JSON value
        typename W::MapFrame frame;
        std::size_t size = 0;
        if (!writer.write_map_begin(size, frame)) return false;
        return writer.write_map_end(frame);
    }
    
    using Opts = typename options::detail::annotation_meta_getter<T>::options;
    
    // Handle WireSink types - they accept any JSON value (empty schema)
    if constexpr (WireSinkLike<T>) {
        // Empty schema {} allows any JSON value
        typename W::MapFrame frame;
        std::size_t size = 0;
        if (!writer.write_map_begin(size, frame)) return false;
        return writer.write_map_end(frame);
    }
    // Handle transformers - use their wire type
    else if constexpr (ParseTransformerLike<T>) {
        return WriteSchemaImpl<typename T::wire_type, SeenTypes...>(writer);
    } 
    // Handle nullable types - emit oneOf [schema, null]
    else if constexpr (NullableParsableValue<T>) {
        auto wr = [&]<class InnerT>(InnerT) constexpr {
            typename W::MapFrame frame;
            std::size_t size = 1;
            if (!writer.write_map_begin(size, frame)) return false;
            if (!writer.write_string("oneOf", 5, true)) return false;
            if (!writer.move_to_value(frame)) return false;
            
            typename W::ArrayFrame arr_frame;
            std::size_t arr_size = 2;
            if (!writer.write_array_begin(arr_size, arr_frame)) return false;
            
            // First alternative: the inner type
            if (!WriteSchemaImpl<InnerT, SeenTypes...>(writer)) return false;
            
            // Second alternative: null
            if (!writer.advance_after_value(arr_frame)) return false;
            if (!WriteNullSchema(writer)) return false;
            
            if (!writer.write_array_end(arr_frame)) return false;
            return writer.write_map_end(frame);
        };
        if constexpr (input_checks::is_specialization_of_v<T, std::optional>) {
            return wr(typename T::value_type{});
        } else if constexpr (input_checks::is_specialization_of_v<T, std::unique_ptr>) {
            return wr(typename T::element_type{});
        } else {
            static_assert(sizeof(T) == 0, "Bad nullable storage");
            return false;
        }
    } 
    // Handle primitive types
    else if constexpr (BoolLike<T>) {
        return WriteBoolSchema<Type>(writer);
    } else if constexpr (StringLike<T>) {
        return WriteStringSchema<Type>(writer);
    } else if constexpr (NumberLike<T>) {
        return WriteNumberSchema<Type>(writer);
    }
    // Handle container types
    else if constexpr (ParsableArrayLike<T>) {
        return WriteArraySchema<Type, SeenTypes...>(writer);
    } else if constexpr (ParsableMapLike<T>) {
        return WriteMapSchema<Type, SeenTypes...>(writer);
    }
    // Handle object types (structs)
    else {
        return WriteObjectSchema<Type, SeenTypes...>(writer);
    }
    } // end else (no cycle)
}

} // namespace detail

/// Write JSON Schema for type T
/// @tparam T The type to generate schema for (must satisfy ParsableValue)
/// @tparam W The writer type (must satisfy WriterLike concept)
/// @param writer The writer instance to output JSON Schema to
/// @param title Optional title for the schema
/// @param schema_uri Optional $schema URI (defaults to JSON Schema draft 2020-12)
/// @return true on success, false on write error
template <typename T, writer::WriterLike W>
    requires ParsableValue<T>
constexpr bool WriteSchema(W & writer, 
                 const char* title = nullptr,
                 const char* schema_uri = "https://json-schema.org/draft/2020-12/schema") {
    // For V1: if no metadata, just write the schema directly
    if (title == nullptr && schema_uri == nullptr) {
        return detail::WriteSchemaImpl<T>(writer);
    }
    
    // Otherwise, wrap with metadata
    // TODO: Properly merge the type's schema properties into the root instead of nesting
    typename W::MapFrame root_frame;
    std::size_t root_props = 0;
    
    if (schema_uri != nullptr) ++root_props;
    if (title != nullptr) ++root_props;
    ++root_props; // for the definition
    
    if (!writer.write_map_begin(root_props, root_frame)) return false;
    
    bool first = true;
    
    // Write $schema
    if (schema_uri != nullptr) {
        if (!first && !writer.advance_after_value(root_frame)) return false;
        first = false;
        if (!writer.write_string("$schema", 7, true)) return false;
        if (!writer.move_to_value(root_frame)) return false;
        if (!writer.write_string(schema_uri, std::strlen(schema_uri), true)) return false;
    }
    
    // Write title
    if (title != nullptr) {
        if (!first && !writer.advance_after_value(root_frame)) return false;
        first = false;
        if (!writer.write_string("title", 5, true)) return false;
        if (!writer.move_to_value(root_frame)) return false;
        if (!writer.write_string(title, std::strlen(title), true)) return false;
    }
    
    // Write the actual schema under a $defs entry for now (V1 limitation)
    // A better approach would be to inline the schema properties at root level
    if (!first && !writer.advance_after_value(root_frame)) return false;
    if (!writer.write_string("definition", 10, true)) return false;
    if (!writer.move_to_value(root_frame)) return false;
    if (!detail::WriteSchemaImpl<T>(writer)) return false;
    
    return writer.write_map_end(root_frame);
}

// Simplified version without $schema and title wrapper
template <typename T, writer::WriterLike W>
    requires ParsableValue<T>
constexpr bool WriteSchemaInline(W & writer) {
    return detail::WriteSchemaImpl<T>(writer);
}

} // namespace JsonFusion::json_schema

