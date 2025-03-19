cmake_minimum_required(VERSION 3.0.2)

project(lib)

set (CMAKE_CXX_STANDARD 17)


option(BUILD_PQRS_BINS "Build and install pqrs example binaries." ON)

add_subdirectory(ext-deps/xtl)
add_subdirectory(ext-deps/xtensor)
add_subdirectory(ext-deps/dynamic_bitset)

add_subdirectory(lib)

if (BUILD_PQRS_BINS)
    add_subdirectory(bin)
endif()

if (EMSCRIPTEN)
    message("EMSCRIPTEN DETECTED!!")
    add_subdirectory(emscripten/cpp)
    # add CMakeLists.txt to run npm build?
endif()

