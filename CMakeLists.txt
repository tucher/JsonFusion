cmake_minimum_required(VERSION 3.5)

project(JsonFusion VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Single include directory containing both JsonFusion/ and pfr/
include_directories(${PROJECT_SOURCE_DIR}/include)


file(GLOB JSON_FUSION
     ${PROJECT_SOURCE_DIR}/include/JsonFusion/*.hpp
)
add_executable(misc_tests
    tests/misc/main.cpp
    ${JSON_FUSION}
)

add_executable(rapidjson_comparison_parsing_benchmark
    benchmarks/comparison_parsing_benchmark.cpp
    tests/test_model.hpp
    ${JSON_FUSION}

)
target_include_directories(rapidjson_comparison_parsing_benchmark PRIVATE
  ../Downloads/rapidjson-master/include
  ../Downloads/json-develop/single_include
)

target_compile_options(rapidjson_comparison_parsing_benchmark PRIVATE
  $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-flto>
)

target_link_options(rapidjson_comparison_parsing_benchmark PRIVATE
  $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-flto>
)

add_executable(consteval_tests
    tests/consteval_tests.cpp
    ${JSON_FUSION}

)
