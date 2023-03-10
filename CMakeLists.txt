cmake_minimum_required(VERSION 3.16.0)
project(node-clone VERSION 0.1.0)

add_library(duktape
    duk/duktape.c
    duk/duk_module_duktape.c
)

add_executable(nodec
    src/main.c
    src/cwalk.c
    src/evloop.c
    src/hashmap.c
    src/loop.c
    src/message.c
    src/pool.c
    src/server.c
    src/task.c
    src/worker.c
)

target_compile_options(nodec PRIVATE -Werror -Wall)
target_compile_options(duktape PRIVATE -std=c99)

target_include_directories(nodec PRIVATE src duk)
target_include_directories(duktape PRIVATE duk)

target_link_libraries(nodec PRIVATE duktape pthread)
target_link_libraries(duktape PRIVATE m)