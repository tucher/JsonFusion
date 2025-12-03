cmake_minimum_required(VERSION 3.5)

project(JsonFusion VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Single include directory containing both JsonFusion/ and pfr/
include_directories(${PROJECT_SOURCE_DIR}/include ../Downloads/rapidjson-master/include)


file(GLOB JSON_FUSION
     ${PROJECT_SOURCE_DIR}/include/JsonFusion/*.hpp
)


# add_executable(rapidjson_comparison_parsing_benchmark
#     benchmarks/comparison_parsing_benchmark.cpp
#     tests/test_model.hpp
#     ${JSON_FUSION}

# )
# target_include_directories(rapidjson_comparison_parsing_benchmark PRIVATE
#   ../Downloads/rapidjson-master/include
# )

# target_compile_options(rapidjson_comparison_parsing_benchmark PRIVATE
#   $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-flto>
# )

# target_link_options(rapidjson_comparison_parsing_benchmark PRIVATE
#   $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-flto>
# )

# add_executable(consteval_tests
#     tests/consteval_tests.cpp
#     ${JSON_FUSION}

# )

add_executable(canada_json_parsing
    # tests/sax_demo.cpp
    benchmarks/canada_json/canada_json_parsing.cpp
    benchmarks/canada_json/canada_json_parsing.hpp
    benchmarks/canada_json/canada_json_parsing_rapidjson.cpp
    ${JSON_FUSION}

)

target_compile_options(canada_json_parsing PRIVATE
  $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-flto>
)

target_link_options(canada_json_parsing PRIVATE
  $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-flto>
)
target_compile_options(canada_json_parsing PRIVATE -fno-exceptions -fno-rtti)


# target_include_directories(sax_demo PRIVATE
#   ../Downloads/rapidjson-master/include
# )

add_executable(benchmarks
    benchmarks/syntetic/main.cpp
    benchmarks/syntetic/bench_matrix.hpp
    # benchmarks/syntetic/benchmarks_jsonfusion_tu.cpp
    # benchmarks/syntetic/benchmarks_jsonfusion_tu.hpp
    benchmarks/syntetic/benchmarks_rapidjson_tu.cpp
    benchmarks/syntetic/benchmarks_rapidjson_tu.hpp
    benchmarks/syntetic/benchmarks_models.hpp
    ${JSON_FUSION}

)
target_include_directories(benchmarks PRIVATE
  ../Downloads/rapidjson-master/include
  ../Downloads/json-develop/single_include
)



target_compile_options(benchmarks PRIVATE
  $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-flto>
)
target_link_options(benchmarks PRIVATE
  $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-flto>
)
target_compile_options(benchmarks PRIVATE -fno-exceptions -fno-rtti)





add_executable(twitter_json_parsing
    benchmarks/twitter_json/twitter_model.hpp
    benchmarks/twitter_json/twitter_json_parsing.cpp
    benchmarks/twitter_json/rapidjson_populate.hpp
    benchmarks/twitter_json/twitter_json_parsing_rapidjson.cpp
    benchmarks/twitter_json/benchmark.hpp
    ${JSON_FUSION}

)
target_include_directories(twitter_json_parsing PRIVATE
  ../Downloads/rapidjson-master/include
)

target_compile_options(twitter_json_parsing PRIVATE
  $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-flto>
)

target_link_options(twitter_json_parsing PRIVATE
  $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-flto>
)
