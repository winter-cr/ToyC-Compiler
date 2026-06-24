cmake_minimum_required(VERSION 3.16)
project(ToyC-Compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(toyc
    src/main.cpp
    src/lexer/Token.cpp
    src/lexer/Lexer.cpp
    src/ast/Type.cpp
    src/ast/Expr.cpp
    src/ast/Decl.cpp
    src/ast/Stmt.cpp
    src/ast/FuncDef.cpp
    src/ast/CompUnit.cpp
    src/ast/AstPrinter.cpp
    src/parser/Parser.cpp
    src/semantic/Symbol.cpp
    src/semantic/SymbolTable.cpp
    src/semantic/SemanticAnalyzer.cpp
    src/semantic/ConstExprEvaluator.cpp
)

target_include_directories(toyc PRIVATE src ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(toyc PRIVATE /W4 /permissive-)
else()
    target_compile_options(toyc PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_test(
    NAME smoke_basic_output
    COMMAND ${CMAKE_COMMAND}
        -DTOYC_EXECUTABLE=$<TARGET_FILE:toyc>
        -DTEST_INPUT=${CMAKE_SOURCE_DIR}/tests/basic/return_7.tc
        -DTEST_OUTPUT=${CMAKE_BINARY_DIR}/smoke_return_7.s
        -P ${CMAKE_SOURCE_DIR}/scripts/cmake/smoke_test.cmake
)
