# ToyC Backend

该模块将后端 IR 转换为 RISC-V 32 位 RV32IM 汇编。它可以作为 C++ 库接入
前端，也可以通过文本 IR 命令行独立运行。

## 环境

基础构建需要：

- CMake 3.16 或更高版本
- 支持 C++20 的编译器
- Ninja（默认生成器）

真实 RV32 端到端测试额外需要：

- `riscv64-unknown-elf-gcc`
- `qemu-riscv32`

在 Ubuntu/WSL 中常见的基础环境安装方式：

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build qemu-user
```

RISC-V bare-metal GCC 的软件包名称随 Ubuntu 版本而异。如果系统软件源没有
提供，需要单独安装 RISC-V GNU Toolchain。

## 构建和测试

```bash
cd backend
make
make test
make e2e
make clean
```

`make test` 会运行 C++ 单元测试；检测到交叉编译器和 QEMU 后，还会运行真实
RV32 程序。端到端测试依次执行：

```text
文本 IR → 优化 → RV32IM 汇编 → ELF 链接 → QEMU 执行 → 检查退出码
```

## 命令行接口

`toyc_backend_cli` 从标准输入读取文本 IR，向标准输出写汇编：

```bash
make cli < tests/data/factorial.ir > factorial.s
make cli ARGS=-opt < tests/data/factorial.ir > factorial-opt.s
```

支持参数：

- `-opt`：常量折叠、常量分支简化和无用临时值删除。
- `--no-comments`：不在汇编中输出后端注释。
- `-h`、`--help`：显示帮助。

诊断信息写入标准错误，成功生成的汇编只写入标准输出，适合评测流水线重定向。

## 文本 IR 格式

每行一条指令，`#` 后面是注释。值的写法如下：

- `123`、`-4`：32 位立即数
- `%t0`：虚拟临时值
- `%l0`：局部变量或形参
- `@counter`：全局变量

完整指令：

```text
global @name initial-value [const]

function name [void]
param %lN [debug-name]
label label-name
copy destination source
unary destination neg|not operand
binary destination add|sub|mul|div|rem|eq|ne|lt|le|gt|ge left right
jump label-name
branch condition true-label false-label
call destination|- function-name [arguments...]
return [value]
end
```

示例：

```text
function main
binary %t0 mul 6 7
return %t0
end
```

## C++ 集成接口

前端完成语义检查后，应把 AST 降低为
[`Program`](include/toyc/backend/ir.hpp)，再调用：

```cpp
Program program = lowerAstToIr(ast);
program = optimize(std::move(program)); // 仅在 -opt 下调用
std::cout << RiscV32CodeGenerator{}.generate(program);
```

`FunctionBuilder` 提供结构化 lowering 辅助方法：

- `emitIf`：生成 if/else 标签和跳转。
- `emitWhile`：生成循环条件、循环体和结束标签。
- `emitBreak`、`emitContinue`：跳转到当前最内层循环。
- `emitLogicalAnd`、`emitLogicalOr`：按 C 语义生成短路求值控制流。

这些 API 不依赖具体 AST 类型，因此成员 A/B 的 AST 确定后只需编写
`lowerAstToIr` 适配层，不需要修改代码生成器。

## 栈帧和调用约定

- 目标架构为 RV32IM，栈保持 16 字节对齐。
- `a0-a7` 传递前 8 个参数，其余参数由调用者放到栈上。
- 返回值使用 `a0`。
- 每个函数保存 `ra` 和旧 `s0`，`s0` 指向调用者进入函数前的栈顶。
- 虚拟临时值优先分配到 `s1-s11`；函数保存实际使用的寄存器。
- 超过 11 个虚拟临时值时自动溢出到当前栈帧。
- 局部变量保存在栈中，全局变量通过 `la` 加 `lw/sw` 访问。

使用 callee-saved 寄存器保存临时值，可保证普通调用和递归调用期间临时值不被
破坏，同时明显减少原实现对每个临时值执行的 `lw/sw`。

## 当前模块边界

backend 已覆盖 IR、算术与关系运算、控制流、短路逻辑、函数调用、递归、全局
数据、栈帧、参数和返回值。ToyC 源码解析及 AST 语义信息属于前端和语义模块；
完整编译器集成时，由根目录的 compiler 入口读取 ToyC 源码并调用上述 C++ API。
