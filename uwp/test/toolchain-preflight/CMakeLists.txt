cmake_minimum_required(VERSION 3.20)
project(xps3_uwp_toolchain_preflight LANGUAGES C CXX)

add_library(xps3_uwp_toolchain_preflight OBJECT main.cpp)
target_compile_features(xps3_uwp_toolchain_preflight PRIVATE cxx_std_20)
