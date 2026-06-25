# ToyC 编译器实践报告

## 项目概述

本项目实现了一个 ToyC → RISC-V32 汇编的编译器，由四人协作六天完成。编译器从标准输入读取 ToyC 源码，向标准输出写入 RISC-V32 汇编，支持 `-opt` 优化参数。

---

## 一、架构设计

### 1.1 模块划分

```
stdin (ToyC 源码)
    │
    ▼
┌──────────────┐
│    Lexer     │  词法分析：源码 → Token 序列
└──────┬───────┘
       │
       ▼
┌──────────────┐
│    Parser    │  语法分析：Token → AST（递归下降）
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  Semantic    │  语义分析：符号表、作用域、类型检查、常量求值
│  Analyzer    │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│     IR       │  中间表示：AST → 三地址码（toyc::backend::Program）
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  CodeGen     │  代码生成：IR → RISC-V32 汇编
└──────┬───────┘
       │
       ▼
stdout (RISC-V32 汇编)
```

### 1.2 技术选型

- 语言：C++20
- 构建：CMake ≥ 3.16
- 可执行文件：`toyc`
- 输入：stdin
- 输出：stdout（汇编）、stderr（错误/诊断）
- IR/后端：独立 `backend/` 目录，`toyc::backend` 命名空间

---

## 二、测试设计

### 2.1 测试目录结构

```
tests/
├── basic/           # 11 个基础功能测试（表达式、变量、函数、控制流）
├── control_flow/    # 5 个控制流专项测试（嵌套、break、continue）
├── semantic/        # 32 个语义测试
│   ├── valid/       # 16 个合法程序 → 应通过语义分析
│   └── invalid/     # 16 个非法程序 → 应有对应错误码
└── performance/     # 2 个性能基准测试（递归、循环）
```

### 2.2 测试策略

| 层级 | 说明 | 工具 |
|------|------|------|
| 冒烟测试 | 构建后验证编译器能输出非空汇编 | `run_smoke_tests.sh` + CTest |
| 语义单元测试 | 编译期检查合法/非法语义样例 | `tests/test_semantic.cpp` + `tests/build_test.py` |
| 端到端测试 | ToyC → RISC-V → 运行 → 对比退出码 | `run_e2e_tests.sh` |
| 回归测试 | 每修复一个 bug 新增一个最小样例 | 持续积累 |
| 优化回归 | `-opt` 前后退出码一致性检查 | `run_e2e_tests.sh --opt` |
| 后端单元测试 | IR 优化 + RV32 代码生成验证 | `backend/tests/backend_tests.cpp` |

### 2.3 期望值映射

所有测试用例的期望退出码集中在 `tests/basic/expected.tsv`（TSV 格式），便于脚本批量读取和比对。

---

## 三、自动化脚本

### 3.1 冒烟测试（`scripts/run_smoke_tests.sh`）

- CMake 配置 → 构建 → CTest → 编译器编译 `return_7.tc` → 验证输出非空
- Day 1 起可用，CI 已配置（`.github/workflows/ci.yml`）
- 支持 Windows（`.ps1`）和 Unix（`.sh`）

### 3.2 端到端测试（`scripts/run_e2e_tests.sh`）

- 遍历 `expected.tsv`，对每个 `.tc` 文件：编译 → 汇编 → 链接 → 运行 → 对比退出码
- 支持 `--opt` 参数切换优化模式
- 依赖 RISC-V 工具链（`riscv64-unknown-elf-gcc` + `qemu-riscv32`）

### 3.3 冒烟辅助（`scripts/cmake/smoke_test.cmake`）

- CMake 脚本模块，由 CTest 调用
- 校验编译器退出码为 0、输出文件非空

### 3.4 后端构建脚本（`backend/Makefile`）

- 独立后端开发构建：`make` / `make test` / `make e2e` / `make clean`
- 文本 IR 命令行工具：`make cli < tests/data/factorial.ir > factorial.s`
- 支持 `-opt`、`--no-comments`、`-h` 参数

---

## 四、Git 集成流程

### 4.1 分支策略

| 分支 | 成员 | 职责 |
|------|------|------|
| `main` | D（合并） | 稳定分支，始终可构建 |
| `feature/frontend` | A | 词法分析、语法分析、AST |
| `feature/semantic` | B | 符号表、语义分析、常量求值 |
| `backend` | C | IR 设计、RISC-V32 代码生成、优化 |
| `test-opt-report` | D | 测试、优化、报告、集成 |

### 4.2 合并流程

1. 成员在自己的分支上开发并推送
2. D 创建 `integrate/<module>` 分支从功能分支
3. 合并最新 `main` 到 integration 分支、解决冲突
4. 构建验证通过后 `git merge --no-ff` 到 `main`
5. 推送 `main`，成员从 `main` 同步最新代码

### 4.3 CI/CD

- 触发器：PR 到 main、push 到 main/feature 分支、手动触发
- 步骤：checkout → 安装依赖 → 构建 → 冒烟测试
- 端到端 CI 待后端与前端管线完全串联后启用

---

## 五、优化设计

### 5.1 优化 Pass 列表

| 优化 | 说明 | 安全性 | 状态 |
|------|------|--------|------|
| 常量折叠 | 编译期计算常量表达式（含一元/二元运算） | 安全 | 已实现 |
| 常量分支简化 | 静态确定条件时移除无效分支 | 安全 | 已实现 |
| 死代码删除 | 移除无用临时值 | 安全 | 已实现 |
| 基础寄存器复用 | 减少不必要的栈读写 | 需验证 | 已实现 |
| 跳转合并 | 合并连续跳转指令 | 安全 | 计划中 |

### 5.2 优化回归保护

- 所有优化必须通过 `run_e2e_tests.sh --opt` 回归测试
- 优化前后程序退出码必须一致
- 优化可逐项开关（`-opt` 参数下各 Pass 可独立启用/禁用）
- 后端优化器位于 `backend/src/optimizer.cpp`，支持 `-opt` 命令行参数

---

## 六、当前状态与已知限制

### 6.1 模块完成度（Day 4）

| 模块 | 完成度 | 备注 |
|------|--------|------|
| 前端（A） | ~98% | AST 重构完成，字段与 Semantic.md 对齐，AstPrinter/Parser 配套更新 |
| 语义（B） | ~85% | 已适配前端 AST + ASTVisitor，统一 toyc 命名空间，测试用例重组 16+16 |
| 后端（C） | ~90% | IR+代码生成+优化器完整，文本 IR CLI 可用，寄存器复用已实现 |
| 测试/集成（D）| ~80% | 测试基础设施完整，三模块已合并至 main，端到端管线待完全串联 |

### 6.2 已解决的关键阻塞

- **B 语义分析器适配**（Day 4 解决）：SemanticAnalyzer 现使用 `#include "ast/AST.h"`、实现 `toyc::ASTVisitor`、统一 `toyc` 命名空间，不再依赖独立 `semantic/AST.h`
- **AST 统一**（Day 3 解决）：前端 AST 唯一定义在 `src/ast/`，B 独立定义已删除
- **后端接入**（Day 3 解决）：backend 已合并到 main，CMakeLists 整合完成

### 6.3 待处理

- [ ] 端到端管线完全串联（Lexer → Parser → Semantic → IR → CodeGen → 汇编输出）
- [ ] 主 main.cpp 接入 SemanticAnalyzer 和 backend CodeGen
- [ ] 全量端到端测试通过（阶乘、斐波那契、嵌套循环、递归、全局变量）
- [ ] `-opt` 回归测试全部通过
- [ ] 清理临时文件（`.idea/`, `compile_err.txt`, `*.bak`）
- [ ] README 构建和运行命令可复现性验证

---

## 附录 A：团队成员分工

| 成员 | 职责 | 主要交付 |
|------|------|----------|
| A | 前端 | Lexer、Parser、AST、AstPrinter（重构为 ASTBase.h + AST.h） |
| B | 语义分析 | SymbolTable、SemanticAnalyzer（已适配 ASTVisitor）、ConstExprEvaluator |
| C | 后端 | IR（toyc::backend::Program）、RISC-V32 CodeGen、Optimizer、TextIR CLI |
| D | 测试/集成 | 测试脚本、CI、分支合并、报告汇总、优化验证 |

## 附录 B：关键文件索引

| 文件 | 说明 |
|------|------|
| `src/ast/ASTBase.h` | AST 基类（ASTNode, Expr, Stmt）+ SourceLocation |
| `src/ast/AST.h` | 全部 AST 节点定义 + ASTVisitor 接口 |
| `src/ast/Type.h` | 类型系统（Int/Void） |
| `src/semantic/SemanticAnalyzer.h` | 语义分析器（实现 ASTVisitor） |
| `src/semantic/SymbolTable.h` | 嵌套作用域符号表 |
| `src/semantic/ConstExprEvaluator.h` | 常量表达式求值 |
| `backend/include/toyc/backend/ir.hpp` | 后端 IR 定义 |
| `backend/include/toyc/backend/optimizer.hpp` | IR 优化器 |
| `backend/src/riscv32_codegen.cpp` | RISC-V32 代码生成 |
| `Day3更新说明.md` | 前端 AST 字段详解与调用指南 |
| `Semantic.md` | 语义分析模块设计文档 |
| `FrontEnd.md` | 前端实现文档 |
