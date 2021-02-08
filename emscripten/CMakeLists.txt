
add_executable(pqrs-emscripten-wrapper
        wrap.cpp)
target_link_libraries(pqrs-emscripten-wrapper
        pqrs)
target_link_options(pqrs-emscripten-wrapper PRIVATE --bind -sALLOW_MEMORY_GROWTH=1 -sDISABLE_EXCEPTION_CATCHING=0
        -sWASM=0 -sENVIRONMENT=web -sFILESYSTEM=0)