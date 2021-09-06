
set(WRAPPER_SRC wrap.cpp)
set(WRAPPER_LIB pqrs json11)
set(WRAPPER_FLAGS
        --bind
        --memory-init-file=0
        -sALLOW_MEMORY_GROWTH=1
        -sDISABLE_EXCEPTION_CATCHING=0
        -sFILESYSTEM=0
        -sMODULARIZE=1
        -sMALLOC=dlmalloc
        -sASSERTIONS=0)

add_executable(pqrs-emscripten-wrapper-pure ${WRAPPER_SRC})
target_link_libraries(pqrs-emscripten-wrapper-pure PRIVATE "${WRAPPER_LIB}")
target_link_options(pqrs-emscripten-wrapper-pure PRIVATE ${WRAPPER_FLAGS} -sWASM=0)

add_executable(pqrs-emscripten-wrapper-wasm ${WRAPPER_SRC})
target_link_libraries(pqrs-emscripten-wrapper-wasm PRIVATE "${WRAPPER_LIB}")
target_link_options(pqrs-emscripten-wrapper-wasm PRIVATE ${WRAPPER_FLAGS} -sWASM=1)

add_custom_target(pqrs-emscripten-wrapper)
add_dependencies(pqrs-emscripten-wrapper pqrs-emscripten-wrapper-pure pqrs-emscripten-wrapper-wasm)
