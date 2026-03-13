# cmake_minimum_required(VERSION 3.10)
#
# project(SimpleCompiler VERSION 1.0 LANGUAGES CXX)
#
# # Устанавливаем стандарт C++17
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)
#
# add_executable(front
#     main.cc
#     lexer.cc
# )
#
# target_include_directories(front PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
#
# target_compile_options(front PRIVATE -Wall -Wextra -Wpedantic)

set(INC_DIR "include")
set(SRC_DIR "sources")

add_executable(
    frontend
    ${SRC_DIR}/lexer.cc
    ${SRC_DIR}/syntax.cc
    ${SRC_DIR}/main.cc
)

target_include_directories(
    frontend
    PRIVATE
    ${INC_DIR}
)

target_link_libraries(
    frontend
    PRIVATE
    DUMB::common
)

target_compile_options(
    frontend
    PRIVATE
    -Wall
    -Werror
)
