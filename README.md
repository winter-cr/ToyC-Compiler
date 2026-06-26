# ToyC Compiler

ToyC Compiler 是一个课程实践用的 ToyC 到 RV32 汇编编译器。编译器从标准输入读取 ToyC 源程序，向标准输出写出 RISC-V 32 位汇编；错误信息输出到标准错误。

## 快速构建

推荐使用 CMake：

```bash
cmake -S . -B build
cmake --build build
```

也提供根目录 `Makefile`，方便评测系统直接执行 `make`：

```bash
make
```

生成的编译器可执行文件名：

- CMake: `build/toyc`
- Makefile: `./toyc`

## 使用方式

```bash
./build/toyc < tests/performance/loop_sum.tc > output.s
./build/toyc -opt < tests/performance/loop_sum.tc > output.s
```

使用 Makefile 构建后：

```bash
./toyc -opt < tests/performance/fib_heavy.tc > output.s
```

`-opt` 会开启 IR 优化、跳转清理和汇编级窥孔优化。

## 项目结构

```text
.
├── CMakeLists.txt          # CMake 构建入口，默认 Release
├── Makefile                # 通用 make 构建入口，生成 ./toyc
├── README.md               # 项目说明
├── docs/                   # 实践报告和文档
├── scripts/                # 辅助测试脚本
├── src/
│   ├── main.cpp            # 编译器命令行入口
│   ├── lexer/              # 词法分析
│   ├── parser/             # 语法分析和 AST 构建
│   ├── ast/                # AST 节点与打印
│   ├── semantic/           # 语义分析、符号表、常量表达式求值
│   ├── codegen/            # AST 到后端 IR 降低
│   └── optimizer/          # 前端集成层优化
├── backend/
│   ├── include/toyc/backend/ # 后端 IR、优化器、RV32 代码生成接口
│   ├── src/                  # 后端实现
│   ├── tests/                # 后端单元测试
│   └── examples/             # 后端示例
└── tests/                  # 基础、语义、控制流和性能样例
```

## 编译流程

1. `Lexer` 将 ToyC 源码转换为 token 序列。
2. `Parser` 构建 AST。
3. `SemanticAnalyzer` 检查作用域、常量、函数调用、返回路径和控制流合法性。
4. `AstToIr` 将 AST 降低到三地址 IR。
5. `backend::optimize` 和 `DOptimizer` 在 `-opt` 下执行常量折叠、死临时值删除、跳转清理等优化。
6. `RiscV32CodeGenerator` 生成 RV32 汇编。

## 当前优化

- 修复 `x-y` 这类表达式的词法分词，避免把减号误并入标识符。
- 返回路径分析支持嵌套块、完整 `if/else` 和常量真循环。
- IR 死临时值删除改为反向活跃性扫描，避免重复全量扫描导致编译时间增长。
- RV32 后端将常用局部变量、参数和临时值分配到 `s1..s11`，减少循环和递归中的栈访问。
- 复制、返回和条件分支对寄存器值走快速路径，减少不必要的 `mv` 和 `lw/sw`。

## 评测提交建议

提交仓库时请包含以下内容：

- `CMakeLists.txt` 和 `Makefile`
- `src/`、`backend/`、`tests/`、`scripts/`、`docs/`
- `README.md`
- `docs/实践报告.md`

不要提交 `build/`、本地可执行文件、token、`.env` 或 IDE 缓存。