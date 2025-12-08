#pragma once

#include "options.hpp"
#include "parse_result.hpp"

namespace JsonFusion {

namespace error_formatting_detail {
template<class Options>
struct get_opt_string_helper;

// Specialization for field_options
template<class... Opts>
struct get_opt_string_helper<options::detail::field_options<OptionsPack<Opts...>>> {
    static std::string_view get(std::size_t i) {
        // Build a table with all option names
        static const std::string_view table[] = { Opts::to_string()... };
        constexpr std::size_t N = sizeof...(Opts);

        if (i >= N) {
            // Out-of-range handling; you can also throw or assert instead
            return {};
        }
        return table[i];
    }
};


const char* ws = " \t\n\r\f\v";

// trim from end of string (right)
std::string& rtrim(std::string& s, const char* t = ws)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from beginning of string (left)
std::string& ltrim(std::string& s, const char* t = ws)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from both ends of string (right then left)
std::string& trim(std::string& s, const char* t = ws)
{
    return ltrim(rtrim(s, t), t);
}

}

template <class C, class InpIter, class DataIter, std::size_t MaxSchemaDepth, bool HasMaps>
std::string ParseResultToString(const ParseResult<InpIter, MaxSchemaDepth, HasMaps> & res, DataIter inp, const DataIter end, std::size_t window = 40) {
    std::string jsonPath = "$";
    const auto & jp = res.errorJsonPath();
    for(int i = 0; i < jp.currentLength; i ++) {
        if(jp.storage[i].array_index == std::numeric_limits<std::size_t>::max()) {
            jsonPath += "." + std::string(jp.storage[i].field_name);
        } else {
            jsonPath += "["+std::to_string(jp.storage[i].array_index)+"]";
        }
    }
    std::string fragment;
    if constexpr(std::is_convertible_v<InpIter, const char*>) {
        int pos = res.pos() - inp;
        std::string before(
            inp + (pos+1 >= window ? pos+1-window:0),
            inp + (pos+1 >= window ? pos+1-window:0) + (pos+1 >= window ? window:0)
        );
        std::size_t tot = end - inp;
        std::string after(
            inp + (pos+1 < tot ? pos + 1: tot),
            inp + (pos+1 + window < tot ? pos+1 + window: tot)
        );
        error_formatting_detail::trim(before);
        error_formatting_detail::trim(after);
        fragment = std::format(": '...{}😖{}...'", before, after);
    }
    if(res.error() != ParseError::SCHEMA_VALIDATION_ERROR) {
        return std::format("When parsing {}, parsing error '{}'{}", jsonPath, error_to_string(res.error()), fragment);
    } else {
        auto validationResult = res.validationErrors();
        std::size_t index = res.validationErrors().validator_index();
        SchemaError err = res.validationErrors().error();


        std::string valDetail = "";
        using Opts    = options::detail::annotation_meta_getter<C>::options;

        if(!jp.template visit_options<JsonFusion::static_schema::AnnotatedValue<C>> ([&]<class Opts>(Opts){
                if constexpr (!std::is_same_v<Opts, options::detail::no_options>) {
                    std::string_view s = error_formatting_detail::get_opt_string_helper<Opts>::get(index);
                    valDetail = " (" + std::string(s) + ")";
                }
            }, Opts{})) {

        }
        std::string valInfo = std::format("validator #{}{} error: '{}'", index, valDetail, validator_error_to_string(err));
        return std::format("When parsing {}, {}{}", jsonPath, valInfo, fragment);
    }
}


}
