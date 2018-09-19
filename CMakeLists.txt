cmake_minimum_required(VERSION 2.8)
project(final)

set(CMAKE_CXX_STANDARD 11)

include_directories(inc)
include_directories(src)
set(SOURCE_FILES final.cpp ./src/sock_con.cpp)


add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES})
target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_range_for)
