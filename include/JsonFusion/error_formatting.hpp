#pragma once
#include "parser.hpp"

namespace JsonFusion {

template <CharInputIterator InpIter, std::size_t MaxSchemaDepth>

std::string ParseResultToString(const ParseResult<InpIter, MaxSchemaDepth> & res, InpIter inp, const InpIter end, std::size_t window = 40) {
    std::string jsonPath = "$";
    auto jp = res.errorJsonPath();
    for(int i = 1; i < jp.currentLength; i ++) {
        jsonPath += ".";
        if(jp.storage[i].array_index == std::numeric_limits<std::size_t>::max()) {
            jsonPath += jp.storage[i].field_name;
        } else {
            jsonPath += std::to_string(jp.storage[i].array_index);
        }
    }

    int pos = res.offset();
    std::string before(
        inp + (pos+1 >= window ? pos+1-window:0),
        inp + (pos+1 >= window ? pos+1-window:0) + (pos+1 >= window ? window:0)
    );
    std::size_t tot = end - inp;
    std::string after(
        inp + (pos+1 < tot ? pos + 1: tot),
        inp + (pos+1 + window < tot ? pos+1 + window: tot)
    );

    return std::format("When parsing {}, error {}: '...{}😖{}...'", jsonPath, int(res.error()), before, after);
}
}
