#pragma once

#include <string_view>
namespace JsonFusion {


enum class ParseError {
    NO_ERROR,
    ILLFORMED_NUMBER,
    ILLFORMED_NULL,
    ILLFORMED_STRING,
    ILLFORMED_ARRAY,
    ILLFORMED_OBJECT,

    UNEXPECTED_END_OF_DATA,
    EXCESS_CHARACTERS,
    FIXED_SIZE_CONTAINER_OVERFLOW,
    NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE,
    FLOAT_VALUE_IN_INTEGER_STORAGE,
    ILLFORMED_BOOL,
    EXCESS_FIELD,
    NULL_IN_NON_OPTIONAL,

    EXCESS_DATA,
    SKIPPING_STACK_OVERFLOW,
    SCHEMA_VALIDATION_ERROR,

    ARRAY_DESTRUCTURING_SCHEMA_ERROR,
    DATA_CONSUMER_ERROR,
    DUPLICATE_KEY_IN_MAP,
    JSON_SINK_OVERFLOW,
    NON_BOOL_JSON_IN_BOOL_VALUE,
    WRONG_JSON_FOR_NUMBER_STORAGE,
    NON_STRING_IN_STRING_STORAGE,
    NON_ARRAY_IN_ARRAY_LIKE_VALUE,
    NON_OBJECT_IN_MAP_LIKE_VALUE,
    NON_ARRAY_IN_DESTRUCTURED_STRUCT
};

constexpr std::string_view error_to_string(ParseError e) {
    switch(e) {
    case ParseError::NO_ERROR: return "NO_ERROR"; break;
    case ParseError::ILLFORMED_NUMBER: return "ILLFORMED_NUMBER"; break;
    case ParseError::ILLFORMED_NULL: return "ILLFORMED_NULL"; break;
    case ParseError::ILLFORMED_STRING: return "ILLFORMED_STRING"; break;
    case ParseError::ILLFORMED_ARRAY: return "ILLFORMED_ARRAY"; break;
    case ParseError::ILLFORMED_OBJECT: return "ILLFORMED_OBJECT"; break;
    case ParseError::UNEXPECTED_END_OF_DATA: return "UNEXPECTED_END_OF_DATA"; break;
    case ParseError::EXCESS_CHARACTERS: return "EXCESS_CHARACTERS"; break;
    case ParseError::FIXED_SIZE_CONTAINER_OVERFLOW: return "FIXED_SIZE_CONTAINER_OVERFLOW"; break;
    case ParseError::NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE: return "NUMERIC_VALUE_IS_OUT_OF_STORAGE_TYPE_RANGE"; break;
    case ParseError::FLOAT_VALUE_IN_INTEGER_STORAGE: return "FLOAT_VALUE_IN_INTEGER_STORAGE"; break;
    case ParseError::ILLFORMED_BOOL: return "ILLFORMED_BOOL"; break;
    case ParseError::EXCESS_FIELD: return "EXCESS_FIELD"; break;
    case ParseError::NULL_IN_NON_OPTIONAL: return "NULL_IN_NON_OPTIONAL"; break;
    case ParseError::EXCESS_DATA: return "EXCESS_DATA"; break;
    case ParseError::SKIPPING_STACK_OVERFLOW: return "SKIPPING_STACK_OVERFLOW"; break;
    case ParseError::SCHEMA_VALIDATION_ERROR: return "SCHEMA_VALIDATION_ERROR"; break;
    case ParseError::ARRAY_DESTRUCTURING_SCHEMA_ERROR: return "ARRAY_DESTRUCTURING_SCHEMA_ERROR"; break;
    case ParseError::DATA_CONSUMER_ERROR: return "DATA_CONSUMER_ERROR"; break;
    case ParseError::DUPLICATE_KEY_IN_MAP: return "DUPLICATE_KEY_IN_MAP"; break;
    case ParseError::JSON_SINK_OVERFLOW: return "JSON_SINK_OVERFLOW"; break;
    case ParseError::NON_BOOL_JSON_IN_BOOL_VALUE: return "NON_BOOL_JSON_IN_BOOL_VALUE"; break;
    case ParseError::WRONG_JSON_FOR_NUMBER_STORAGE: return "WRONG_JSON_FOR_NUMBER_STORAGE"; break;
    case ParseError::NON_STRING_IN_STRING_STORAGE: return "NON_STRING_IN_STRING_STORAGE"; break;
    case ParseError::NON_ARRAY_IN_ARRAY_LIKE_VALUE: return "NON_ARRAY_IN_ARRAY_LIKE_VALUE"; break;
    case ParseError::NON_OBJECT_IN_MAP_LIKE_VALUE: return "NON_OBJECT_IN_MAP_LIKE_VALUE"; break;
    case ParseError::NON_ARRAY_IN_DESTRUCTURED_STRUCT: return "NON_ARRAY_IN_DESTRUCTURED_STRUCT"; break;


    }
    return "N/A";
}


// ============================================================================
// Schema Errors
// ============================================================================

enum class SchemaError  {
    none                             ,
    number_out_of_range              ,
    string_length_out_of_range       ,
    array_items_count_out_of_range   ,
    missing_required_fields          ,
    map_properties_count_out_of_range,
    map_key_length_out_of_range      ,
    wrong_constant_value             ,
    map_key_not_allowed              ,
    map_key_forbidden                ,
    map_missing_required_key
};

constexpr std::string_view validator_error_to_string(SchemaError e) {
    switch(e) {
    case SchemaError::none                             : return "none"; break;
    case SchemaError::number_out_of_range              : return "number_out_of_range"; break;
    case SchemaError::string_length_out_of_range       : return "string_length_out_of_range"; break;
    case SchemaError::array_items_count_out_of_range   : return "array_items_count_out_of_range"; break;
    case SchemaError::missing_required_fields          : return "missing_required_fields"; break;
    case SchemaError::map_properties_count_out_of_range: return "map_properties_count_out_of_range"; break;
    case SchemaError::map_key_length_out_of_range      : return "map_key_length_out_of_range"; break;
    case SchemaError::wrong_constant_value             : return "wrong_constant_value"; break;
    case SchemaError::map_key_not_allowed              : return "map_key_not_allowed"; break;
    case SchemaError::map_key_forbidden                : return "map_key_forbidden"; break;
    case SchemaError::map_missing_required_key         : return "map_missing_required_key"; break;
    }
    return "N/A";
}

struct ValidationResult {
    SchemaError  m_error  = SchemaError::none;
    std::size_t validatorIndex;
    constexpr operator bool() const {
        return m_error == SchemaError::none;
    }

    constexpr SchemaError error() const {
        return m_error;
    }
    constexpr std::size_t validator_index() const {
        return validatorIndex;
    }
};

} // namespace JsonFusion
