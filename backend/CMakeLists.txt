set(INC_DIR "include")
set(SRC_DIR "sources")

set(
    COMPILE_OPTIONS
    -Wall
    -Werror
    -Wextra
    -Wpedantic
)

set(
    BACKEND_SOURCES
    ${SRC_DIR}/backend.cc
)

set(
    BACKEND_HEADERS
    ${INC_DIR}/backend.hh
)

#
# Separate backend executable
#

# TODO unused

# add_executable(
#     backend
#     ${BACKEND_SOURCES}
#     ${BACKEND_HEADERS}
# )
#
# target_compile_definitions(
#     backend
#     PRIVATE
#     BUILD_BACKEND_SEPARATELY
# )
#
# target_include_directories(
#     backend
#     PRIVATE
#     ${INC_DIR}
# )
#
# target_link_libraries(
#     backend
#     PRIVATE
#     DUMB::common
# )
#
# target_compile_options(
#     backend
#     PRIVATE
#     ${COMPILE_OPTIONS}
# )

#
# Backend library
#

add_library(
    backend_lib
    ${BACKEND_SOURCES}
    ${BACKEND_HEADERS}
)

target_include_directories(
    backend_lib
    PUBLIC
    ${INC_DIR}
)

target_link_libraries(
    backend_lib
    PRIVATE
    DUMB::common
)

target_compile_options(
    backend_lib
    PRIVATE
    ${COMPILE_OPTIONS}
)
