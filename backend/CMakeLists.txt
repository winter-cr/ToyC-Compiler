cmake_minimum_required(VERSION 3.16)
project(toyc_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(toyc_backend
    src/ir.cpp
    src/riscv32_codegen.cpp
)

target_include_directories(toyc_backend PUBLIC include)

add_executable(backend_demo examples/backend_demo.cpp)
target_link_libraries(backend_demo PRIVATE toyc_backend)

add_executable(backend_tests tests/backend_tests.cpp)
target_link_libraries(backend_tests PRIVATE toyc_backend)

enable_testing()
add_test(NAME backend_tests COMMAND backend_tests)
