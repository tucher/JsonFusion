#pragma once

#include <string_view>
namespace JsonFusion {


enum class ParseError {
    NO_ERROR,

    FIXED_SIZE_CONTAINER_OVERFLOW,

    NON_NUMERIC_IN_NUMERIC_STORAGE,
    NON_BOOL_IN_BOOL_VALUE,
    NON_STRING_IN_STRING_STORAGE,
    NON_ARRAY_IN_ARRAY_LIKE_VALUE,
    NON_MAP_IN_MAP_LIKE_VALUE,
    NON_ARRAY_IN_DESTRUCTURED_STRUCT,
    NULL_IN_NON_OPTIONAL,

    EXCESS_FIELD,
    ARRAY_DESTRUCTURING_SCHEMA_ERROR,

    DATA_CONSUMER_ERROR,
    DUPLICATE_KEY_IN_MAP,

    TRANSFORMER_ERROR,
    SCHEMA_VALIDATION_ERROR,
    READER_ERROR
};

constexpr std::string_view error_to_string(ParseError e) {
    switch(e) {
    case ParseError::NO_ERROR: return "NO_ERROR"; break;
    case ParseError::FIXED_SIZE_CONTAINER_OVERFLOW: return "FIXED_SIZE_CONTAINER_OVERFLOW"; break;
    case ParseError::NON_NUMERIC_IN_NUMERIC_STORAGE: return "NON_NUMERIC_IN_NUMERIC_STORAGE"; break;
    case ParseError::EXCESS_FIELD: return "EXCESS_FIELD"; break;
    case ParseError::NULL_IN_NON_OPTIONAL: return "NULL_IN_NON_OPTIONAL"; break;
    case ParseError::SCHEMA_VALIDATION_ERROR: return "SCHEMA_VALIDATION_ERROR"; break;
    case ParseError::ARRAY_DESTRUCTURING_SCHEMA_ERROR: return "ARRAY_DESTRUCTURING_SCHEMA_ERROR"; break;
    case ParseError::DATA_CONSUMER_ERROR: return "DATA_CONSUMER_ERROR"; break;
    case ParseError::DUPLICATE_KEY_IN_MAP: return "DUPLICATE_KEY_IN_MAP"; break;
    case ParseError::NON_BOOL_IN_BOOL_VALUE: return "NON_BOOL_IN_BOOL_VALUE"; break;
    case ParseError::NON_STRING_IN_STRING_STORAGE: return "NON_STRING_IN_STRING_STORAGE"; break;
    case ParseError::NON_ARRAY_IN_ARRAY_LIKE_VALUE: return "NON_ARRAY_IN_ARRAY_LIKE_VALUE"; break;
    case ParseError::NON_MAP_IN_MAP_LIKE_VALUE: return "NON_MAP_IN_MAP_LIKE_VALUE"; break;
    case ParseError::NON_ARRAY_IN_DESTRUCTURED_STRUCT: return "NON_ARRAY_IN_DESTRUCTURED_STRUCT"; break;
    case ParseError::TRANSFORMER_ERROR: return "TRANSFORMER_ERROR"; break;
    case ParseError::READER_ERROR: return "READER_ERROR"; break;

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
    map_missing_required_key         ,
    user_defined_fn_validator_error,
    forbidden_fields
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
    case SchemaError::user_defined_fn_validator_error  : return "user_defined_fn_validator_error"; break;
    case SchemaError::forbidden_fields                 : return "forbidden_fields"; break;
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
