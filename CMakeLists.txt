cmake_minimum_required(VERSION 3.16)

project(CASLockFreeStackBenchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cas_stack_benchmark
    src/main.cpp
)

target_include_directories(cas_stack_benchmark PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

find_package(Threads REQUIRED)
target_link_libraries(cas_stack_benchmark PRIVATE Threads::Threads)

if (MSVC)
    target_compile_options(cas_stack_benchmark PRIVATE /W4)
else()
    target_compile_options(cas_stack_benchmark PRIVATE -Wall -Wextra)
endif()