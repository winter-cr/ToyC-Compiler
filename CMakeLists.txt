cmake_minimum_required(VERSION 3.16)
project(ToyC-Compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(toyc
    src/main.cpp
    src/lexer/Token.cpp
    src/lexer/Lexer.cpp
    src/ast/Expr.cpp
    src/ast/Decl.cpp
    src/ast/Stmt.cpp
    src/ast/FuncDef.cpp
    src/ast/CompUnit.cpp
    src/ast/AstPrinter.cpp
    src/parser/Parser.cpp
)

target_include_directories(toyc PRIVATE src)

if(MSVC)
    target_compile_options(toyc PRIVATE /W4 /permissive-)
else()
    target_compile_options(toyc PRIVATE -Wall -Wextra -Wpedantic)
endif()
