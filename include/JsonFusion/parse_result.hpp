#pragma once

#include <string_view>

#include "io.hpp"
#include "json_path.hpp"
#include "parse_errors.hpp"

namespace JsonFusion {


template <CharInputIterator InpIter, std::size_t SchemaDepth, bool SchemaHasMaps>
class ParseResult {
    ParseError m_error = ParseError::NO_ERROR;
    InpIter m_begin, m_pos;
    ValidationResult validationResult;


    json_path::JsonPath<SchemaDepth, SchemaHasMaps> currentPath;

public:
    constexpr ParseResult(ParseError err, ValidationResult schemaErrors, InpIter begin, InpIter pos, json_path::JsonPath<SchemaDepth, SchemaHasMaps> jsonP):
        m_error(err), m_begin(begin), m_pos(pos), validationResult(schemaErrors), currentPath(jsonP)
    {}
    constexpr operator bool() const {
        return m_error == ParseError::NO_ERROR && validationResult;
    }
    constexpr InpIter pos() const {
        return m_pos;
    }
    constexpr std::size_t offset() const {
        return m_pos - m_begin;
    }
    constexpr ParseError error() const {
        if (m_error == ParseError::NO_ERROR) {
            return ParseError::NO_ERROR;
        } else {
            if(!validationResult) {
                return ParseError::SCHEMA_VALIDATION_ERROR;
            }
        }
        return m_error;
    }
    constexpr const json_path::JsonPath<SchemaDepth, SchemaHasMaps> & errorJsonPath() const {
        return currentPath;
    }
    constexpr ValidationResult validationErrors() const {
        return validationResult;
    }
};

template <class M, CharInputIterator It>
struct ModelParsingTraits {
    static constexpr std::size_t SchemaDepth = schema_analyzis::calc_type_depth<M>();
    static constexpr bool SchemaHasMaps = schema_analyzis::has_maps<M>();
    using ResultT = ParseResult<It,SchemaDepth, SchemaHasMaps>;
};

template <class M, CharInputIterator It>
using ParseResultT = ModelParsingTraits<M, It>::ResultT;


} // namespace JsonFusion
