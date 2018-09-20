cmake_minimum_required(VERSION 2.8)
project(final)
##project(main)


macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
    endif ()
  else ()
    set (CMAKE_CXX_STANDARD 11)
  endif ()
endmacro(use_cxx11)

include_directories(inc)
include_directories(src)
set(SOURCE_FILES final.cpp ./src/sock_con.cpp)
##set(SOURCE_FILES main.cpp)


add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES})
TARGET_LINK_LIBRARIES( final pthread)
