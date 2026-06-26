# ToyC Backend

本目录是成员 C 的独立交付模块，包含三地址 IR、RISC-V32 汇编生成、栈帧布局、
函数调用约定、最小示例和后端单元测试。它不依赖具体 AST 类型；前端与语义分析
完成后，只需把合法程序降低为 `toyc::backend::Program`。

## 目录

```text
backend/
├── CMakeLists.txt
├── include/toyc/backend/
│   ├── ir.hpp
│   └── riscv32_codegen.hpp
├── src/
│   ├── ir.cpp
│   └── riscv32_codegen.cpp
├── examples/backend_demo.cpp
└── tests/backend_tests.cpp
```

## IR 设计

IR 是显式控制流的三地址码：

- `Value`：立即数、虚拟寄存器、局部变量、全局变量。
- `Copy`：赋值及变量读写。
- `Unary`：负号、逻辑非。
- `Binary`：加减乘除模及六种关系运算。
- `Label`、`Jump`、`Branch`：控制流和短路求值。
- `Call`：函数调用，可选返回值。
- `Return`：有值或无值返回。

逻辑 `&&` 和 `||` 应由 AST 降低阶段转为标签和条件跳转，从而保持短路语义。例如
`a && b` 可降低为：

```text
branch a, rhs, false
rhs:
branch b, true, false
true:
result = 1
jump end
false:
result = 0
end:
```

`FunctionBuilder` 提供参数、局部变量、临时值和唯一标签的分配接口。语义层负责保证
类型、常量和控制流合法，`validate()` 只检查后端所需的基本结构，如重复符号和未定义
标签。

## RV32 指令选择

| IR | RV32 指令 |
| --- | --- |
| 加、减、乘、除、模 | `add`、`sub`、`mul`、`div`、`rem` |
| 相等、不等 | `xor` + `seqz/snez` |
| 小于、大于 | `slt`（必要时交换操作数） |
| 小于等于、大于等于 | `slt` + `xori 1` |
| 逻辑非 | `seqz` |
| 条件跳转 | `bnez` + `j` |
| 函数调用 | `call` |
| 返回 | `ret` |

当前生成目标为带 M 扩展的 RV32 GNU 汇编。每条 IR 指令使用 `t0`、`t1` 加载操作数，
使用 `t2` 保存计算结果；虚拟寄存器和局部变量拥有固定栈槽。这一方案优先保证正确性，
后续可在 `-opt` 模式加入基本块内寄存器复用。

## 栈帧

栈向低地址增长，帧大小按 16 字节对齐。函数执行期间：

```text
高地址
  s0 + 0     调用者为第 9 个及以后参数保留的区域
  s0 - 4     保存的 ra
  s0 - 8     保存的旧 s0
  s0 - 12    第一个局部量/虚拟寄存器槽
  ...        其余 4 字节栈槽
低地址
  sp         当前栈帧底部（16 字节对齐）
```

`s0` 始终指向进入函数时的 `sp`。因此函数内部为了调用其他函数而临时移动 `sp` 时，
局部变量位置不会变化，递归调用也能正确工作。

## 调用约定

采用 RISC-V ILP32 常规整数调用方式：

- 前 8 个 `int` 参数通过 `a0` 到 `a7` 传递。
- 第 9 个及以后参数由调用者放到栈上，首个栈参数位于调用时的 `0(sp)`。
- 整数返回值放在 `a0`。
- `ra` 和 `s0` 由被调用者保存并恢复。
- 调用者的额外参数区按 16 字节对齐，调用结束后立即回收。

## 构建和测试

```powershell
cmake -S backend -B backend/build
cmake --build backend/build
ctest --test-dir backend/build --output-on-failure
```

运行示例会向标准输出打印一个包含递归阶乘和全局变量写入的 RV32 汇编程序：

```powershell
backend/build/backend_demo > factorial.s
```

## 与 AST 集成

推荐由前端团队提供只读 AST，成员 C 编写单独的 lowering visitor：

1. 遇到变量声明时调用 `createLocal()`，并发出初始化 `Copy`。
2. 普通表达式返回一个 `Value`；复杂表达式通过 `createTemporary()` 保存结果。
3. `if/while/break/continue` 使用 `createLabel()`、`Branch` 和 `Jump`。
4. `&&/||` 使用条件上下文降低，不先计算右操作数。
5. 全局常量可直接替换成立即数；全局变量使用 `Value::global(name)`。
6. 完成 `Program` 后调用 `RiscV32CodeGenerator::generate()`，将结果写到 stdout。

