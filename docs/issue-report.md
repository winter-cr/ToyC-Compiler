# ToyC 编译器 — 集成修复 Issue 报告

> 统计区间：`b706b8b` → `90417b2`（2026-06-25，D 代行集成修复全记录）  
> 提交数：30 个  
> 当前状态：CI 构建通过，23/23 端到端测试通过（等待最终 CI 验证）

---

## 一、项目 vs 任务要求符合性检查

| 要求 | 状态 | 说明 |
|------|:---:|------|
| stdin 读取 ToyC 源码 | ✅ | `main.cpp` 使用 `std::cin` |
| stdout 输出 RISC-V32 汇编 | ✅ | 完整管线 Lexer→Parser→Semantic→IR→CodeGen→stdout |
| 支持 `-opt` 参数 | ✅ | `main.cpp` 解析 `-opt`，传递至 `backend::optimize()` |
| C++20 + CMake 构建 | ✅ | `CMakeLists.txt` 完整，含所有源文件 |
| 仓库可被评测系统克隆构建 | ✅ | 已推送 main，CI 验证通过 |
| 功能测试 1s 时限 | ✅ | 所有 23 用例通过 |
| 实践报告 | ✅ | `docs/report.md` 226 行，结构完整 |
| 不依赖现成后端框架 | ✅ | 自行实现 IR+CodeGen+Optimizer |
| 全局变量/常量支持 | ✅ | IR 支持 Global，代码生成支持 |
| 短路逻辑 `&&` / `||` | ✅ | `FunctionBuilder::emitLogicalAnd/Or` |
| 函数调用/递归 | ✅ | `fib_heavy.tc` (递归) 通过 |
| break/continue | ✅ | `break_inner.tc` / `continue_loop.tc` 通过 |

---

## 二、D 成员个人任务完成情况

### Day 1-3 ✅ 全部完成

| 任务 | 状态 |
|------|:---:|
| 测试目录结构（basic/semantic/control_flow/performance） | ✅ |
| 最小 ToyC 样例 | ✅ |
| 测试脚本（smoke + e2e） | ✅ |
| Git 合并规则，D 统一合并到 main | ✅ |
| 拉取并测试 feature/frontend | ✅ |
| 表达式/if/while/嵌套 block 样例 | ✅ |
| 语义测试扩充（15 valid + 16 invalid） | ✅ |
| 合并 feature/semantic | ✅ |

### Day 4 ✅ 已完成

| 任务 | 状态 |
|------|:---:|
| 端到端测试脚本 `run_e2e_tests.sh` | ✅ |
| -nostdlib + 自定义 _start（避免 64/32 CRT 不兼容） | ✅ |
| -static 静态链接（避免 PIE 动态链接） | ✅ |
| --build-id=none（解决 section 重叠） | ✅ |
| CI 第二阶段：RISC-V 工具链 + e2e 测试 | ✅ |
| 端到端测试全部通过（23/23） | ✅ |

### Day 5 ⚠️ 部分完成

| 任务 | 状态 |
|------|:---:|
| CI 第三阶段：-opt 回归测试 | ✅ 已配置 |
| -opt 回归测试**实际通过** | ⚠️ 待 CI 验证 |
| 优化效果记录（汇编行数/运行时间） | ❌ 无数据 |

### Day 6 ❌ 未完成

| 任务 | 状态 |
|------|:---:|
| 全量测试通过 | ⚠️ 功能 23/23 通过，语义测试未接入 CI |
| README 构建命令可复现性验证 | ❌ |
| 实践报告补充测试结果和优化数据 | ⚠️ 框架完整，缺数据 |
| 清理临时文件 | ❌ |

### 超额完成（D 代行）

| 任务 | 原责任人 |
|------|:---:|
| B 模块接口适配（4 文件） | B |
| AST→IR 转换器实现 | C |
| 主 CMakeLists 集成 backend/codegen | C |
| 完整编译管线串联 | C |
| CI 三阶段配置 | D（原计划） |

---

## 三、Git 历史关键 Issue 分析

### Issue 1：SemanticAnalyzer visit 签名不匹配

| 项目 | 内容 |
|------|------|
| **提交** | `8557972` |
| **现象** | `visit(CompUnit*) marked override but does not override` — 19 个方法报错 |
| **根因** | A 的 Day3 AST 重构将 `ASTVisitor` 的 visit 签名从 `visit(X* node)` 改为 `visit(const X& node)`。B 的 `SemanticAnalyzer.h/.cpp` 未同步更新 |
| **应由谁修复** | **B**（语义分析模块）。A 修改了公共接口，B 作为接口消费者必须适配 |
| **D 代行修复** | 重写 `.h`/`.cpp`：18 个方法签名改为 `const T&`，移除 `EmptyStmt` |
| **预防措施** | 公共接口变更需同步通知所有消费者，或让 A 同时更新 B 的代码 |

### Issue 2：SemanticAnalyzer 旧字段名/枚举/Type 方法

| 项目 | 内容 |
|------|------|
| **提交** | `5cda252` |
| **现象** | `node->condition`、`node->thenBranch`、`isInt()` 等不存在 |
| **根因** | B 的 `SemanticAnalyzer.cpp` 使用旧 AST 字段名（`condition`/`thenBranch`/`elseBranch`/`arguments`）、旧嵌套枚举（`BinaryExpr::Op::DIV`）、旧 Type 方法（`isInt()`→应为 `is_int()`） |
| **应由谁修复** | **B**。B 没有 rebase 到 A 最新 AST 上开发 |
| **D 代行修复** | 字段名映射（6 类）、枚举值更新（PascalCase）、Type 方法改为 snake_case |
| **预防措施** | 成员开发前必须先 `git merge main` 同步最新接口 |

### Issue 3：ConstExprEvaluator 旧枚举值

| 项目 | 内容 |
|------|------|
| **提交** | `5cda252` |
| **现象** | `BinaryExpr::Op::ADD`、`UnaryExpr::Op::PLUS` 等不存在 |
| **根因** | B 的 `ConstExprEvaluator.cpp` 使用旧嵌套枚举 `BinaryExpr::Op::ADD`，前端已改为命名空间级 `BinaryOp::Add` |
| **应由谁修复** | **B** |
| **D 代行修复** | 全部替换为 `BinaryOp::Add/Sub/...` 和 `UnaryOp::Plus/Minus/Not` |
| **预防措施** | 同 Issue 2 |

### Issue 4：SymbolTable 头文件与实现不一致

| 项目 | 内容 |
|------|------|
| **提交** | `5cda252` |
| **现象** | `.h` 在 `namespace semantic` 声明 `enterScope()`，`.cpp` 在 `namespace toyc` 实现 `pushScope()` |
| **根因** | B 的 SymbolTable.h 和 SymbolTable.cpp 来自两个不同版本，未同步 |
| **应由谁修复** | **B** |
| **D 代行修复** | 重写 `.h`：统一 `namespace toyc`，方法名匹配 `.cpp` 实现 |
| **预防措施** | 同 Issue 2 |

### Issue 5：AST→IR 转换器缺失

| 项目 | 内容 |
|------|------|
| **提交** | `5cda252`（新建 `src/codegen/AstToIr.cpp/.h`） |
| **现象** | 编译管线在 Parser 后断开，无代码将前端 AST 转为后端 IR |
| **根因** | C 设计了 IR 和 `FunctionBuilder`（含 `emitIf`/`emitWhile` 等），但未实现遍历 AST 生成 IR 的代码 |
| **应由谁修复** | **C**（后端）。IR 是 C 的设计，`FunctionBuilder` 的 `emitIf`/`emitWhile` 等高层 API 就是为 AST→IR 准备的 |
| **D 代行修复** | 实现完整的 `ASTVisitor`，将全部 AST 节点映射到 IR 指令 |
| **预防措施** | C 在设计 IR 时应同时提供 AST→IR 的至少一个示例（如 `backend_demo.cpp`），并明确接口约定 |

### Issue 6：AST→IR 转换器 lambda 捕获问题

| 项目 | 内容 |
|------|------|
| **提交** | `e2ee4c3` |
| **现象** | `'node' is not captured` — IfStmt/WhileStmt visitor 中 lambda 编译报错 |
| **根因** | D 编写 AstToIr 时 `emitIf`/`emitWhile` 的 lambda 只写了 `[this]`，未捕获参数 `node` |
| **应由谁修复** | **D**（编写代码时的遗漏） |
| **修复** | `[this]` → `[this, &node]` |

### Issue 7：AST→IR 参数处理顺序 bug

| 项目 | 内容 |
|------|------|
| **提交** | `776654c` |
| **现象** | `function_call.tc` 返回 0 而非 7，`add` 函数参数被忽略（`li t0, 0` 而非从栈加载参数） |
| **根因** | D 的 `visit(const FuncDef&)` 中 `builder_` 在参数循环**之后**才赋值，导致 `visit(const Param&)` 中的 `if (builder_)` 为 false，参数从未添加到 IR |
| **应由谁修复** | **D**（编写代码时的逻辑 bug） |
| **修复** | `builder_ = &funcBuilder` 移至参数循环之前 |

### Issue 8：e2e 链接 64/32 CRT 不兼容

| 项目 | 内容 |
|------|------|
| **提交** | `ab7b56a` |
| **现象** | `elf64-littleriscv does not match elf32-littleriscv` — 链接失败 |
| **根因** | `gcc-riscv64-linux-gnu` 是 64 位工具链，其 CRT（crt1.o, libc.a）是 elf64。`-march=rv32im` 只影响代码生成，不能改变链接的 CRT 位数 |
| **应由谁修复** | **D**（测试环境配置问题）。属于 CI/测试脚本层面 |
| **修复** | 使用 `-nostdlib -nostartfiles` + 自定义 `_start.s`（参照 `backend/tests/rv32_e2e.sh`） |

### Issue 9：e2e 链接 PIE 动态链接

| 项目 | 内容 |
|------|------|
| **提交** | `29923ef` |
| **现象** | `Could not open '/lib/ld-linux-riscv32-ilp32.so.1'` — QEMU 找不到 32 位动态链接器 |
| **根因** | gcc 默认生成 PIE 可执行文件（Type: DYN），需要动态链接器。加 `-static` 解决 |
| **应由谁修复** | **D**（测试环境配置） |
| **修复** | 添加 `-static` flag |

### Issue 10：e2e 链接 section 重叠

| 项目 | 内容 |
|------|------|
| **提交** | `8ce5ca3` |
| **现象** | `.note.gnu.build-id LMA overlaps section .text` |
| **根因** | gcc 默认生成 `.note.gnu.build-id` section，与 `-Ttext=0x10000` 的 `.text` 段冲突 |
| **应由谁修复** | **D**（测试环境配置） |
| **修复** | 添加 `-Wl,--build-id=none` |

### Issue 11：测试用例预期值错误

| 项目 | 内容 |
|------|------|
| **提交** | `0a4772a` |
| **现象** | `nested_while.tc` 预期 30 实际 15（3×5=15 正确）；`continue_loop.tc` 预期 6 实际 5（5 个奇数正确）；`loop_sum.tc` 预期 5050 实际 186（`5050&0xFF=186` Unix 退出码截断）；`return_4.tc` 缺少全局变量 `b` 声明 |
| **应由谁修复** | **D**（测试用例编写时计算错误） |
| **修复** | 修正 expected.tsv、修复 return_4.tc 添加全局变量声明 |

---

## 四、责任人汇总

| 成员 | 应负责任 | Issue 数 | 说明 |
|:---:|---|:---:|------|
| **A** | 无 | 0 | 前端 AST 实现完整，接口文档齐全（Day3更新说明.md） |
| **B** | Issue 1-4 | **4** | 未适配 A 的 AST 变更：visit 签名、字段名、枚举值、Type 方法、SymbolTable 不一致。全部由 D 代行修复 |
| **C** | Issue 5 | **1** | AST→IR 转换器未实现，主 CMakeLists 未集成 backend。由 D 代行实现 |
| **D** | Issue 6-11 | **6** | Issue 6-7 为编码遗漏（1 行顺序、1 个 capture）；Issue 8-10 为 CI/测试环境配置；Issue 11 为测试用例编写 |

---

## 五、D 剩余未完成任务

| 优先级 | 任务 | 状态 |
|:---:|---|:---:|
| P1 | -opt 回归测试**实际通过**验证 | CI 已配置，待运行 |
| P2 | 优化效果数据记录（汇编行数、运行时间） | 未做 |
| P2 | README 构建和运行命令可复现性验证 | 未做 |
| P3 | 实践报告补充测试结果和优化数据 | 框架完整，缺数据 |
| P3 | 清理临时文件（`.idea/`, `compile_err.txt`, `*.bak`） | 未做 |
| P3 | 语义单元测试接入 CI | 未做 |

---

## 六、当前项目评分预估

| 评分项 | 权重 | 预估 | 说明 |
|------|:---:|:---:|------|
| 功能得分 | 75% | ~90%+ | 23/23 功能测试通过，语法覆盖完整 |
| 性能得分 | 25% | 未知 | 需 OJ 平台运行才能获得性能数据 |
| 实践报告 | 20% | ~70% | 架构/测试/CI/优化章节完整，缺具体测试数据和优化数据 |

> **总得分预估**：需等 OJ 平台评测结果。功能得分高，性能取决于 `-opt` 优化效果和 OJ 基准时间对比。
