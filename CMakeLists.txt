cmake_minimum_required(VERSION 3.5)

project(cpp_struct_serialisation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include_directories(${PROJECT_SOURCE_DIR}/3party)
#add_compile_options(-fconcepts-diagnostics-depth=4)

add_executable(cpp_struct_serialisation
    test_utils.hpp
    main2.cpp
    cpp_json_reflection.hpp
    static_schema.hpp
    parser.hpp
    string_ops.hpp
    # canada_json_perf_test.cpp
    # twitter_json_perf_test.cpp
)

add_executable(rapidjson_comparison_parsing_benchmark
    rapidjson_comparison_parsing_benchmark.cpp
    test_utils.hpp
    static_schema.hpp
    parser.hpp
)
target_include_directories(rapidjson_comparison_parsing_benchmark PRIVATE ../Downloads/rapidjson-master/include)

target_compile_options(rapidjson_comparison_parsing_benchmark PRIVATE
  $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-flto>
)

target_link_options(rapidjson_comparison_parsing_benchmark PRIVATE
  $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-flto>
)
