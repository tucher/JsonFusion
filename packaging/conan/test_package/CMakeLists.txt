cmake_minimum_required(VERSION 3.15)
project(test_package CXX)

find_package(JsonFusion REQUIRED CONFIG)

add_executable(test_package test_package.cpp)
target_link_libraries(test_package JsonFusion::JsonFusion)
target_compile_features(test_package PUBLIC cxx_std_23)
