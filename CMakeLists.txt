cmake_minimum_required(VERSION 3.15)
project(DUMB VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CXX_COMPILE_OPTIONS
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

#
# DotGraphLib for dumping
#
add_subdirectory(dot-graph-lib)

#
# Include and sources directories
#
set(INC_DIR "include")
set(SRC_DIR "sources")

#
# Common build files
#
set(COMMON_INC_DIR "${INC_DIR}/common")
set(COMMON_SRC_DIR "${SRC_DIR}/common")

file(GLOB COMMON_INC "${COMMON_INC_DIR}/*.hh")
file(GLOB COMMON_SRC "${COMMON_SRC_DIR}/*.cc")

#
# Frontend build files
#
set(FRONTEND_INC_DIR "${INC_DIR}/frontend")
set(FRONTEND_SRC_DIR "${SRC_DIR}/frontend")

file(GLOB FRONTEND_INC "${FRONTEND_INC_DIR}/*.hh")
file(GLOB FRONTEND_SRC "${FRONTEND_SRC_DIR}/*.cc")

#
# Backend build files
#
set(BACKEND_INC_DIR "${INC_DIR}/backend")
set(BACKEND_SRC_DIR "${SRC_DIR}/backend")

file(GLOB BACKEND_INC "${BACKEND_INC_DIR}/*.hh")
file(GLOB BACKEND_SRC "${BACKEND_SRC_DIR}/*.cc")

#
# Common build rules
#
add_library(common ${COMMON_SRC})
target_compile_options(common PRIVATE ${CXX_COMPILE_OPTIONS})

target_include_directories(common PUBLIC ${COMMON_INC_DIR})

target_link_libraries(common PRIVATE DotGraphLib)

#
# Frontend build rules
#
add_library(frontend ${FRONTEND_SRC})
target_compile_options(frontend PRIVATE ${CXX_COMPILE_OPTIONS})

target_include_directories(frontend PUBLIC ${FRONTEND_INC_DIR})

target_link_libraries(frontend PUBLIC common)

target_link_libraries(frontend PRIVATE DotGraphLib)

#
# Backend build rules
#
add_library(backend ${BACKEND_SRC})
target_compile_options(backend PRIVATE ${CXX_COMPILE_OPTIONS})

target_include_directories(backend PUBLIC ${BACKEND_INC_DIR})

target_link_libraries(backend PUBLIC common)

#
# DUMB translator build rules
#
add_executable(dumb ${SRC_DIR}/dumb.cc)
target_compile_options(dumb PRIVATE ${CXX_COMPILE_OPTIONS})

target_link_libraries(dumb PRIVATE frontend)

target_link_libraries(dumb PRIVATE backend)
