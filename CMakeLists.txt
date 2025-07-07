cmake_minimum_required(VERSION 3.16)
project(LockFreeQueueBenchmark CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# google benchmark
include(FetchContent)
set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMAKR_ENABLE_GTESTS_TESTS OFF)
FetchContent_Declare(
    benchmark 
    GIT_REPOSITORY https://github.com/google/benchmark.git 
    GIT_TAG main
)
FetchContent_MakeAvailable(benchmark)

# taskflow
FetchContent_Declare(
    taskflow
    GIT_REPOSITORY https://github.com/taskflow/taskflow.git 
    GIT_TAG master
)
FetchContent_MakeAvailable(taskflow)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")

# sources
add_executable(LFQ main.cpp node.h chase_lev_q.h lf_queue.h tf_queue.h LFQ.h)
# target_link_libraries(LFQ PRIVATE taskflow)
target_include_directories(LFQ PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/taskflow-src")
target_compile_options(LFQ PRIVATE -Wall -Wextra -O3)


add_executable(comparing comparing.cpp node.h utils.h tf_queue.h)
target_include_directories(comparing PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/taskflow-src")

# target_link_libraries(comparing PRIVATE taskflow)
target_compile_options(comparing PRIVATE -Wall -Wextra)

# target_compile_definitions(LFQ PRIVATE BENCHMARK_USE_STD_MAIN)

add_executable(benchmark2 benchmark2.cpp utils.h)
target_link_libraries(benchmark2 benchmark::benchmark)
target_include_directories(benchmark2 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/taskflow-src")
