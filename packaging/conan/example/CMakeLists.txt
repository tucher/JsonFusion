cmake_minimum_required(VERSION 3.15)
project(JsonFusionConsumerExample CXX)

find_package(JsonFusion REQUIRED CONFIG)

add_executable(consumer_app main.cpp)
target_link_libraries(consumer_app JsonFusion::JsonFusion)
target_compile_features(consumer_app PUBLIC cxx_std_23)

# Add any other compiler flags if needed
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(consumer_app PRIVATE -Wall -Wextra)
endif()
