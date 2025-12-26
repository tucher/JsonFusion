#include <yyjson.h>
#include <string>
#include "benchmark.hpp"

void yyjson_parse(int iterations, const std::string& json_data) {
    
    benchmark("yyjson parsing", iterations, [&]() {
        // reflect-cpp requires a string copy since it parses from string_view
        std::string copy = json_data;
        
        yyjson_read_err err;
        // According to yyjson's doc, it's safe castaway constness as long as
        // YYJSON_READ_INSITU is not set
        yyjson_doc* doc = yyjson_read_opts(const_cast<char*>(copy.data()),
                                            copy.size(), 0, NULL, &err);
        if (!doc) {
            throw std::string(err.msg);
        }
        yyjson_val* root = yyjson_doc_get_root(doc);
        yyjson_doc_free(doc);
    });
}

