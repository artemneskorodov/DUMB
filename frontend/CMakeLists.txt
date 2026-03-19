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
    FRONTEND_SOURCES
    ${SRC_DIR}/lexer.cc
    ${SRC_DIR}/syntax.cc
    ${SRC_DIR}/frontend.cc
    ${SRC_DIR}/ast_dump.cc
    ${SRC_DIR}/emit_ir.cc
)

set(
    FRONTEND_HEADERS
    ${INC_DIR}/ast_dump.hh
    ${INC_DIR}/ast.hh
    ${INC_DIR}/frontend.hh
    ${INC_DIR}/lexer.hh
    ${INC_DIR}/syntax.hh
    ${INC_DIR}/nametable.hh
    ${INC_DIR}/emit_ir.hh
)

#
# Separate frontend executable
#

add_executable(
    frontend
    ${FRONTEND_SOURCES}
    ${FRONTEND_HEADERS}
)

target_compile_definitions(
    frontend
    PRIVATE
    BUILD_FRONTEND_SEPARATELY
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

target_link_libraries(
    frontend
    PRIVATE
    DotGraphLib
)

target_compile_options(
    frontend
    PRIVATE
    ${COMPILE_OPTIONS}
)

#
# Frontend library
#

add_library(
    frontend_lib
    ${FRONTEND_SOURCES}
    ${FRONTEND_HEADERS}
)

target_include_directories(
    frontend_lib
    PUBLIC
    ${INC_DIR}
)

target_link_libraries(
    frontend_lib
    PRIVATE
    DUMB::common
)

target_link_libraries(
    frontend_lib
    PRIVATE
    DotGraphLib
)

target_compile_options(
    frontend_lib
    PRIVATE
    ${COMPILE_OPTIONS}
)
