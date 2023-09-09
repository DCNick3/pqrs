cmake_minimum_required(VERSION 3.9)

project(pqrs-android)
set (CMAKE_CXX_STANDARD 17)

# add the pqrs library itself
add_subdirectory(../.. pqrs)

find_library(log-lib log)


add_library(pqrs-android SHARED
        src/main/cpp/pqrs-android.cpp
)
target_link_libraries(pqrs-android PRIVATE pqrs json11)
target_link_libraries(pqrs-android PRIVATE ${log-lib})