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

---

## 二、测试设计

### 2.1 测试目录结构

```
tests/
├── basic/           # 11 个基础功能测试（表达式、变量、函数、控制流）
├── control_flow/    # 5 个控制流专项测试（嵌套、break、continue）
├── semantic/        # 32 个语义测试（B 提供：17 合法 + 15 非法）
│   ├── valid/       # 合法程序 → 应通过语义分析
│   └── invalid/     # 非法程序 → 应有对应错误码
└── performance/     # 2 个性能基准测试（递归、循环）
```

### 2.2 测试策略

| 层级 | 说明 | 工具 |
|------|------|------|
| 冒烟测试 | 构建后验证编译器能输出非空汇编 | `run_smoke_tests.sh` + CTest |
| 端到端测试 | ToyC → RISC-V → 运行 → 对比退出码 | `run_e2e_tests.sh` |
| 回归测试 | 每修复一个 bug 新增一个最小样例 | 持续积累 |
| 优化回归 | `-opt` 前后退出码一致性检查 | `run_e2e_tests.sh --opt` |

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
- 依赖 RISC-V 工具链（`riscv64-linux-gnu-gcc` + `qemu-riscv32`）

### 3.3 冒烟辅助（`scripts/cmake/smoke_test.cmake`）

- CMake 脚本模块，由 CTest 调用
- 校验编译器退出码为 0、输出文件非空

---

## 四、Git 集成流程

### 4.1 分支策略

| 分支 | 成员 | 职责 |
|------|------|------|
| `main` | D（合并） | 稳定分支，始终可构建 |
| `feature/frontend` | A | 词法分析、语法分析、AST |
| `feature/semantic` | B | 符号表、语义分析、常量求值 |
| `backend` | C | IR 设计、RISC-V32 代码生成 |
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
- 端到端 CI 待后端就绪后启用

---

## 五、优化设计（计划）

### 5.1 优化 Pass 列表

| 优化 | 说明 | 安全性 |
|------|------|--------|
| 常量折叠 | 编译期计算常量表达式 | 安全 |
| 死代码删除 | 移除不可达基本块 | 安全 |
| 跳转合并 | 合并连续跳转指令 | 安全 |
| 简单寄存器复用 | 减少栈读写 | 需验证 |

### 5.2 优化回归保护

- 所有优化必须通过 `run_e2e_tests.sh --opt` 回归测试
- 优化前后程序退出码必须一致
- 优化可逐项开关（`-opt` 参数下各 Pass 可独立启用/禁用）

---

## 六、当前状态与已知限制

### 6.1 模块完成度（Day 3 末）

| 模块 | 完成度 | 备注 |
|------|--------|------|
| 前端（A） | ~95% | 词法+语法+AST 完整，文档齐全 |
| 语义（B） | ~60% | 逻辑完整但使用独立 AST，待适配前端 AstVisitor |
| 后端（C） | ~80% | IR+代码生成完整，待接入主构建和前端 AST |
| 测试/集成（D）| ~75% | 测试基础设施完整，待各模块管线打通 |

### 6.2 已知阻塞

- B 的 SemanticAnalyzer 使用 `src/semantic/AST.h`（独立定义），未适配 A 的 `src/ast/` + AstVisitor
- C 的后端在独立 `backend/` 目录，命名空间和构建系统未与主项目统一
- 端到端管线（Lexer → Parser → Semantic → IR → CodeGen）尚未串联

### 6.3 待处理

- [ ] B 适配前端 AstVisitor（参见 `SemanticIntegration.md`）
- [ ] C 将后端迁移到 `src/codegen/`、接入主 CMakeLists
- [ ] 统一 include 路径和可执行文件名
- [ ] 清理临时文件（`.idea/`, `build_err.txt`, `*.bak`）
- [ ] 全量端到端测试通过

---

## 附录：团队成员分工

| 成员 | 职责 | 主要交付 |
|------|------|----------|
| A | 前端 | Lexer、Parser、AST、AstVisitor |
| B | 语义分析 | SymbolTable、SemanticAnalyzer、ConstExprEvaluator |
| C | 后端 | IR 设计、RISC-V32 CodeGen、调用约定 |
| D | 测试/集成 | 测试脚本、CI、分支合并、报告汇总、优化验证 |
