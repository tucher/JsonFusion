#pragma once

#include <string_view>

#include "io.hpp"
#include "json_path.hpp"
#include "errors.hpp"

namespace JsonFusion {


template <class InpIter, class ReaderError, std::size_t SchemaDepth, bool SchemaHasMaps>
class ParseResult {
    ParseError m_error = ParseError::NO_ERROR;
    ReaderError m_readerError{};
    InpIter m_pos;
    ValidationResult validationResult;


    json_path::JsonPath<SchemaDepth, SchemaHasMaps> currentPath;

public:
    using iterator_type = InpIter;
    constexpr ParseResult(ParseError err, ReaderError rerr, ValidationResult schemaErrors, InpIter pos, json_path::JsonPath<SchemaDepth, SchemaHasMaps> jsonP):
        m_error(err), m_readerError(rerr), m_pos(pos), validationResult(schemaErrors), currentPath(jsonP)
    {}
    constexpr operator bool() const {
        return m_error == ParseError::NO_ERROR && validationResult;
    }
    constexpr InpIter pos() const {
        return m_pos;
    }

    constexpr ParseError error() const {
        return m_error;
    }
    constexpr ReaderError readerError() const {
        return m_readerError;
    }
    constexpr const json_path::JsonPath<SchemaDepth, SchemaHasMaps> & errorJsonPath() const {
        return currentPath;
    }
    constexpr ValidationResult validationErrors() const {
        return validationResult;
    }
};

template <class M>
struct ModelParsingTraits {
    static constexpr std::size_t SchemaDepth = schema_analyzis::calc_type_depth<M>();
    static constexpr bool SchemaHasMaps = schema_analyzis::has_maps<M>();
};



} // namespace JsonFusion
