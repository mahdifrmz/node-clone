cmake_minimum_required(VERSION 3.18.0)
project(noudjs VERSION 0.1.0)

add_library(duktape
    duk/duktape.c
    duk/duk_module_duktape.c
)

add_executable(noud
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

target_compile_options(noud PRIVATE -Werror -Wall)
target_compile_options(duktape PRIVATE -std=c99)

target_include_directories(noud PRIVATE src duk)
target_include_directories(duktape PRIVATE duk)

target_link_libraries(noud PRIVATE duktape)
target_link_libraries(duktape PRIVATE m)