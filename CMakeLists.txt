cmake_minimum_required(VERSION 3.20)

project(ToyCCompiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(toyc
    src/main.cpp
)

enable_testing()

add_test(
    NAME smoke_basic_output
    COMMAND ${CMAKE_COMMAND}
        -DTOYC_EXECUTABLE=$<TARGET_FILE:toyc>
        -DTEST_INPUT=${CMAKE_SOURCE_DIR}/tests/basic/return_7.tc
        -DTEST_OUTPUT=${CMAKE_BINARY_DIR}/smoke_return_7.s
        -P ${CMAKE_SOURCE_DIR}/scripts/cmake/smoke_test.cmake
)
