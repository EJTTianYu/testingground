cmake_minimum_required(VERSION 3.19)
project(testingground)

set(CMAKE_CXX_STANDARD 20)

# coroutine learn
add_executable(coroutine src/coroutine/main.cc)
# io_uring learn
# add_executable(io_uring src/io_uring/async_write_coroutine.cc)
