cmake_minimum_required(VERSION 3.20)
project(ToyCCompiler VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 如果使用MSVC，禁用一些警告
if(MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 可执行文件
add_executable(compiler
    src/main.cpp
    src/semantic/Symbol.cpp
    src/semantic/SymbolTable.cpp
    src/semantic/SemanticAnalyzer.cpp
    # TODO: 添加其他模块的源文件
)

target_include_directories(compiler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 测试目标（可选）
enable_testing()

# 添加测试
add_test(NAME semantic_empty_main
    COMMAND ${CMAKE_COMMAND} -E echo "int main(void) { return 0; }" | $<TARGET_FILE:compiler>
)

# 测试函数：验证测试样例
function(add_semantic_test test_name test_file expected_exit_code)
    add_test(NAME ${test_name}
        COMMAND ${CMAKE_COMMAND} -E echo "$<JOIN:<, $<FILE:${test_file}>>" | $<TARGET_FILE:compiler>
    )
    set_tests_properties(${test_name} PROPERTIES
        PASS_REGULAR_EXPRESSION ""
        FAIL_REGULAR_EXPRESSION ""
    )
endfunction()

# 可以手动运行以下命令测试：
# ctest -R semantic_ -V
