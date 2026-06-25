cmake_minimum_required(VERSION 3.16)
project(toyc_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(toyc_backend
    src/ir.cpp
    src/optimizer.cpp
    src/riscv32_codegen.cpp
    src/text_ir.cpp
)

target_include_directories(toyc_backend PUBLIC include)

add_executable(backend_demo examples/backend_demo.cpp)
target_link_libraries(backend_demo PRIVATE toyc_backend)

add_executable(toyc_backend_cli tools/backend_cli.cpp)
target_link_libraries(toyc_backend_cli PRIVATE toyc_backend)

add_executable(backend_tests tests/backend_tests.cpp)
target_link_libraries(backend_tests PRIVATE toyc_backend)

enable_testing()
add_test(NAME backend_tests COMMAND backend_tests)

find_program(RISCV_GCC
    NAMES riscv64-unknown-elf-gcc
    HINTS /opt/riscv/bin
)
find_program(QEMU_RISCV32
    NAMES qemu-riscv32
    HINTS /opt/riscv/bin
)

if(RISCV_GCC AND QEMU_RISCV32)
    add_test(
        NAME backend_rv32_e2e
        COMMAND sh
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/rv32_e2e.sh
            $<TARGET_FILE:toyc_backend_cli>
            ${RISCV_GCC}
            ${QEMU_RISCV32}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/factorial.ir
    )
else()
    message(STATUS
        "RV32 end-to-end test disabled: install riscv64-unknown-elf-gcc "
        "and qemu-riscv32 to enable it")
endif()
