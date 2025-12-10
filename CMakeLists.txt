cmake_minimum_required(VERSION 3.10)
project(CP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(test
    src/main.cpp
    src/buddy_allocator.cpp
    src/mckusick_karels_allocator.cpp
    src/benchmark.cpp
)

if(UNIX)
    target_link_libraries(test PRIVATE pthread)
endif()
