cmake_minimum_required(VERSION 3.15)
project(DUMB VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(dot-graph-lib)
add_subdirectory(common)
add_subdirectory(frontend)
add_subdirectory(backend)

add_executable(
    dumb
    dumb.cc
)

target_link_libraries(dumb PRIVATE common)
target_link_libraries(dumb PRIVATE frontend_lib)
target_link_libraries(dumb PRIVATE backend_lib)
