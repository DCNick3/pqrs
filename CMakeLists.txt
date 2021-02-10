cmake_minimum_required(VERSION 3.0.2)

project(lib)

set (CMAKE_CXX_STANDARD 17)


#include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
#conan_basic_setup()

add_subdirectory(ext-deps/xtl)
add_subdirectory(ext-deps/xtensor)
add_subdirectory(ext-deps/dynamic_bitset)

add_subdirectory(lib)
add_subdirectory(demo)

if (EMSCRIPTEN)
    message("EMSCRIPTEN DETECTED!!")
    add_subdirectory(emscripten)
endif()

