# ToyC-Compiler

ToyC-Compiler 是一个面向课程实践的 ToyC 编译器项目。项目目标是将 ToyC 源程序经过词法分析、语法分析、语义分析和代码生成，编译为可执行语义正确的 RISC-V32 汇编代码。

## 项目功能

- 从标准输入 `stdin` 读取 ToyC 源码。
- 向标准输出 `stdout` 输出 RISC-V32 汇编。
- 支持 ToyC 的函数、全局变量、全局常量、局部变量、局部常量、表达式、条件分支、循环、`break`、`continue` 和 `return`。
- 支持嵌套作用域、函数调用、递归、短路逻辑运算和常量表达式编译期求值。
- 支持可选参数 `-opt`，用于开启优化模式。
- 生成代码的运行结果以 `main` 函数返回值为准。

## ToyC 语言范围

ToyC 是 C 语言的简化子集，不包含数组、指针、I/O 和多文件编译。源程序必须包含一个返回类型为 `int`、无参数、名称为 `main` 的入口函数。

支持的核心语法包括：

- `int` / `void` 函数定义
- `int` 变量声明与初始化
- `const int` 常量声明与初始化
- 赋值语句、表达式语句和空语句
- `if` / `else`
- `while`
- `break` / `continue`
- `return`
- 算术、关系和逻辑表达式
- 函数调用

## 技术路线

计划采用 C++20 + CMake 实现，模块划分如下：

```text
src/
├── main.cpp          # 编译器入口，处理 stdin/stdout 和 -opt
├── lexer/            # 词法分析
├── parser/           # 语法分析与 AST 构建
├── ast/              # AST 节点定义
├── semantic/         # 语义分析、符号表、常量求值
├── ir/               # 中间表示
└── codegen/          # RISC-V32 汇编生成
```

## 构建方式

项目需要提供 `CMakeLists.txt`。在仓库根目录执行：

```powershell
cmake -S . -B build
cmake --build build
```

构建完成后，编译器可执行文件预计位于：

```text
build/toyc
```

具体文件名以 `CMakeLists.txt` 中配置为准。

## 运行方式

从标准输入读取 ToyC 文件，并将汇编写入标准输出：

```powershell
build\toyc < tests\basic\example.tc > output.s
```

开启优化模式：

```powershell
build\toyc -opt < tests\basic\example.tc > output.s
```

在 Linux 或评测环境中可使用：

```bash
./build/toyc < tests/basic/example.tc > output.s
./build/toyc -opt < tests/basic/example.tc > output.s
```

## 示例

输入 ToyC 程序：

```c
int main() {
    return 1 + 2 * 3;
}
```

运行：

```powershell
build\toyc < tests\basic\return_7.tc > output.s
```

输出为 RISC-V32 汇编，后续可由测试脚本汇编、链接并运行，程序退出码应为 `7`。

## 测试建议

建议测试流程：

1. 使用 ToyC 编译器生成 RISC-V32 汇编。
2. 将同一份 ToyC 源码作为 C 程序用 GCC 编译运行，记录退出码。
3. 汇编并运行生成的 RISC-V32 程序。
4. 比较两个程序的退出码是否一致。

重点测试类型：

- 表达式优先级和短路逻辑
- 局部变量、全局变量和常量
- 嵌套作用域和同名屏蔽
- if/else 和 while
- break 和 continue
- 函数调用、参数传递和递归
- int/void 返回语义
- `-opt` 开启前后的结果一致性

## 评测接口要求

- 输入：标准输入 `stdin`
- 输出：标准输出 `stdout`
- 错误信息：可输出到 `stderr`
- 优化参数：评测系统可能传入 `-opt`
- 输出内容：RISC-V32 汇编代码

## 开发计划

四人六天开发排期与分工见 [开发任务与分工.md](./开发任务与分工.md)。
