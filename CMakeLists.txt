cmake_minimum_required(VERSION 3.14)  # FetchContent requires 3.11+, using 3.14 for better support

project(JsonFusion VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fetch external dependencies for benchmarking
include(FetchContent)

# Fetch RapidJSON
message(STATUS "Fetching RapidJSON...")
FetchContent_Declare(
  rapidjson
  GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
  GIT_TAG        master  # or use a specific tag like v1.1.0
  GIT_SHALLOW    TRUE
)

# RapidJSON is header-only, just make it available
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(rapidjson)
message(STATUS "RapidJSON fetched successfully")

# Fetch reflect-cpp
message(STATUS "Fetching reflect-cpp...")
FetchContent_Declare(
  reflectcpp
  GIT_REPOSITORY https://github.com/getml/reflect-cpp.git
  GIT_TAG        v0.21.0  # Use a specific stable version
  GIT_SHALLOW    TRUE     # Only download the latest commit for faster fetch
)

# Configure reflect-cpp with JSON support only (minimal dependencies)
set(REFLECTCPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(REFLECTCPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(reflectcpp)
message(STATUS "reflect-cpp fetched successfully")

# Fetch Glaze
message(STATUS "Fetching Glaze...")
FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG        main  # Use main branch for latest
  GIT_SHALLOW    TRUE
)

# Configure Glaze (header-only, minimal config)
set(glaze_DEVELOPER_MODE OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glaze)
message(STATUS "Glaze fetched successfully")
# Single include directory containing both JsonFusion/ and pfr/
include_directories(${PROJECT_SOURCE_DIR}/include)


file(GLOB JSON_FUSION
     ${PROJECT_SOURCE_DIR}/include/JsonFusion/*.hpp
)


# add_executable(rapidjson_comparison_parsing_benchmark
#     benchmarks/comparison_parsing_benchmark.cpp
#     tests/test_model.hpp
#     ${JSON_FUSION}

# )


# target_compile_options(rapidjson_comparison_parsing_benchmark PRIVATE
#   $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-flto=8>
# )

# target_link_options(rapidjson_comparison_parsing_benchmark PRIVATE
#   $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-flto=8>
# )

add_executable(consteval_tests
    tests/consteval_tests.cpp
    include/yyjson/yyjson.c

    ${JSON_FUSION}

)
add_executable(sax_demo
    tests/sax_demo.cpp
    ${JSON_FUSION}

)
add_executable(canada_json_parsing
    # tests/sax_demo.cpp
    benchmarks/canada_json/canada_json_parsing.cpp
    benchmarks/canada_json/canada_json_parsing.hpp
    benchmarks/canada_json/canada_json_parsing_rapidjson.cpp
    include/yyjson/yyjson.c

    ${JSON_FUSION}

)

target_compile_options(canada_json_parsing PRIVATE
  $<$<CONFIG:Release>:
    -fno-exceptions
    -fno-rtti
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
      -flto=8
      -ffunction-sections
      -fdata-sections
    >
  >
)


target_link_options(canada_json_parsing PRIVATE
  $<$<CONFIG:Release>:
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-flto=8>
    # GNU ld / lld
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<NOT:$<PLATFORM_ID:Darwin>>>:-Wl,--gc-sections>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Linux>>:-Wl,--gc-sections>
    # Apple ld
    $<$<PLATFORM_ID:Darwin>:-Wl,-dead_strip>
  >
)

target_include_directories(canada_json_parsing PRIVATE
  ${rapidjson_SOURCE_DIR}/include
)




add_executable(benchmarks
    benchmarks/syntetic/main.cpp
    benchmarks/syntetic/bench_matrix.hpp
    # benchmarks/syntetic/benchmarks_jsonfusion_tu.cpp
    benchmarks/syntetic/benchmarks_jsonfusion_tu.hpp
    benchmarks/syntetic/benchmarks_rapidjson_tu.cpp
    benchmarks/syntetic/benchmarks_rapidjson_tu.hpp
    benchmarks/syntetic/benchmarks_models.hpp
    ${JSON_FUSION}

)
target_include_directories(benchmarks PRIVATE
  ${rapidjson_SOURCE_DIR}/include
)


target_compile_options(benchmarks PRIVATE
  $<$<CONFIG:Release>:
    -fno-exceptions
    -fno-rtti
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
      -flto=8
      -ffunction-sections
      -fdata-sections
    >
  >
)


target_link_options(benchmarks PRIVATE
  $<$<CONFIG:Release>:
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-flto=8>
    # GNU ld / lld
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<NOT:$<PLATFORM_ID:Darwin>>>:-Wl,--gc-sections>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Linux>>:-Wl,--gc-sections>
    # Apple ld
    $<$<PLATFORM_ID:Darwin>:-Wl,-dead_strip>
  >
)


add_executable(twitter_json_parsing
    benchmarks/twitter_json/twitter_model_generic.hpp
    benchmarks/twitter_json/twitter_json_parsing.cpp
    benchmarks/twitter_json/rapidjson_populate.hpp
    benchmarks/twitter_json/twitter_json_parsing_rapidjson.cpp
    benchmarks/twitter_json/reflectcpp_parsing.hpp
    benchmarks/twitter_json/twitter_json_parsing_reflectcpp.cpp
    benchmarks/twitter_json/glaze_parsing.hpp
    benchmarks/twitter_json/twitter_json_parsing_glaze.cpp
    benchmarks/twitter_json/benchmark.hpp
    benchmarks/twitter_json/twiiter_json_parsing_yyjson.cpp
    include/yyjson/yyjson.c
    ${JSON_FUSION}

)
target_include_directories(twitter_json_parsing PRIVATE
  ${rapidjson_SOURCE_DIR}/include
)

# Link reflect-cpp and Glaze for benchmarking
target_link_libraries(twitter_json_parsing PRIVATE reflectcpp glaze::glaze)

target_compile_options(twitter_json_parsing PRIVATE
  $<$<CONFIG:Release>:

    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
      -flto=8
      -ffunction-sections
      -fdata-sections
    >
  >
)


target_link_options(twitter_json_parsing PRIVATE
  $<$<CONFIG:Release>:
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-flto=8>
    # GNU ld / lld
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<NOT:$<PLATFORM_ID:Darwin>>>:-Wl,--gc-sections>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Linux>>:-Wl,--gc-sections>
    # Apple ld
    $<$<PLATFORM_ID:Darwin>:-Wl,-dead_strip>
  >
)


add_executable(fp_test
    tests/fp/main.cpp
    ${JSON_FUSION}

)
