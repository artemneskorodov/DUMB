file(GLOB_RECURSE COMMON_SOURCES "sources/*.cc")
file(GLOB_RECURSE COMMON_HEADERS "include/*.hh")

add_library(
    common
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
)

add_library(
    DUMB::common
    ALIAS
    common
)

target_include_directories(
    common
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(
    common
    PRIVATE
    -Wall
    -Werror
    -Wpedantic
)
