cmake_minimum_required(VERSION 3.16)
project(LockFreeQueueBenchmark CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# google benchmark
include(FetchContent)
# FetchContent_Declare(
#     benchmark 
#     GIT_REPOSITORY https://github.com/google/benchmark.git 
#     GIT_TAG main
# )
# FetchContent_MakeAvailable(benchmark)

# taskflow
FetchContent_Declare(
    taskflow
    GIT_REPOSITORY https://github.com/taskflow/taskflow.git 
    GIT_TAG master
)
FetchContent_MakeAvailable(taskflow)

# sources
add_executable(LFQ main.cpp node.h chase_lev_q.h lf_queue.h tf_queue.h LFQ.h)
# target_link_libraries(LFQ PRIVATE taskflow)
target_include_directories(LFQ PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/taskflow-src")
target_compile_options(LFQ PRIVATE -Wall -Wextra -O3)

# target_compile_definitions(LFQ PRIVATE BENCHMARK_USE_STD_MAIN)
