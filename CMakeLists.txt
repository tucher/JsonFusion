cmake_minimum_required(VERSION 3.5)

project(cpp_struct_serialisation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include_directories(${PROJECT_SOURCE_DIR}/3party)
#add_compile_options(-fconcepts-diagnostics-depth=4)

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
