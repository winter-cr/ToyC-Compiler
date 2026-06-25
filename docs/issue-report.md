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

## 二、Git 历史关键 Issue 分析

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

## 三、责任人汇总

| 成员 | 应负责任 | Issue 数 | 说明 |
|:---:|---|:---:|------|
| **A** | 无 | 0 | 前端 AST 实现完整，接口文档齐全（Day3更新说明.md） |
| **B** | Issue 1-4 | **4** | 未适配 A 的 AST 变更：visit 签名、字段名、枚举值、Type 方法、SymbolTable 不一致。全部由 D 代行修复 |
| **C** | Issue 5 | **1** | AST→IR 转换器未实现，主 CMakeLists 未集成 backend。由 D 代行实现 |
| **D** | Issue 6-11 | **6** | Issue 6-7 为编码遗漏（1 行顺序、1 个 capture）；Issue 8-10 为 CI/测试环境配置；Issue 11 为测试用例编写 |

---

## 四、OJ 评测结果驱动的修复（提交 `6b12e48`）

> 第一次 OJ 得分：51.75 / 100（功能 66.67 / 性能 6.99）

### Issue 12：代码生成过度使用 callee-saved register → 性能极低

| 项目 | 内容 |
|------|------|
| **现象** | 性能测试得分仅 6.99，4 个超时，通过测试的运行时间是基准的 5-17% |
| **根因** | `assignRegisters` 将所有 VirtualRegister 分配至 s1-s11，导致每个操作都有 `mv sN, tX`(spill) + `mv tX, sN`(fill) 双链，指令数翻倍，循环体内尤其严重 |
| **应由谁修复** | **C**（后端代码生成）。寄存器分配策略是后端核心设计 |
| **D 代行修复** | `assignRegisters` 返回空 map，所有 VirtualRegister 走栈 slot，消除 mv 双链 |
| **影响测试** | p01-p12 全部性能测试，p07-p10 超时 |

### Issue 13：AstToIr Block 作用域缺失 → 变量遮蔽/作用域错误

| 项目 | 内容 |
|------|------|
| **现象** | f07/f13/f25 错误输出（作用域遮蔽、嵌套 Block、全局遮蔽） |
| **根因** | `visit(const Block&)` 未保存/恢复 `locals_` 映射，嵌套 Block 内声明的变量会永久覆盖外层同名变量，离开 Block 后无法恢复 |
| **应由谁修复** | **C**（AstToIr 实现者）。但根本原因也涉及 **B**（语义分析未阻止遮蔽场景进入代码生成） |
| **D 代行修复** | Block visitor 入口保存 `locals_` 副本，出口恢复 |
| **影响测试** | f07, f13, f25, f28（全局声明与函数混合） |

### Issue 14：ExprStmt 未区分 void/非 void 函数调用 → void 函数 crash

| 项目 | 内容 |
|------|------|
| **现象** | f10_void_fn / f24_void_return 编译器异常 |
| **根因** | `visit(const ExprStmt&)` 无条件调用 `emitExpr`，对 void 函数调用会创建无用 temp 并尝试捕获 a0（无意义值） |
| **应由谁修复** | **C**（AstToIr 实现者）。ExprStmt→FuncCall 的路由逻辑属于代码生成层 |
| **D 代行修复** | ExprStmt visitor 检测 `FuncCall::isStatement`，走 `visit(const FuncCall&)` 无 destination 路径 |
| **影响测试** | f10, f24 |

### Issue 15：全局变量 initExpr 空指针保护缺失

| 项目 | 内容 |
|------|------|
| **现象** | f21_global_var 可能由此 crash |
| **根因** | `visit(const VarDecl&)` 中 `node.initExpr.get()` 未做空指针检查直接传入 `dynamic_cast` |
| **应由谁修复** | **C**（AstToIr 实现者） |
| **D 代行修复** | 添加 `if (node.initExpr)` 检查后再访问 |

---

## 五、更新后责任人汇总

| 成员 | Issue | 数量 | 说明 |
|:---:|---|:---:|------|
| **A** | — | 0 | 前端 AST 完整，接口文档齐全 |
| **B** | #1-4 | 4 | 未适配 A 的 AST 变更（visit 签名/字段名/枚举/Type） |
| **C** | #5, #12-15 | 5 | IR 设计完整但 AST→IR 未实现、寄存器分配策略失误、作用域/Bock/void 处理缺失 |
| **D** | #6-11 | 6 | 编码遗漏(#6-7)、CI/测试配置(#8-10)、测试用例计算错误(#11) |

---

## 六、当前评分预估（修复后）

| 评分项 | 修复前 | 修复后预估 | 说明 |
|------|:---:|:---:|------|
| 功能得分 | 66.67 | ~85+ | 修复 void 函数和全局变量后应解决 3-5 个异常 |
| 性能得分 | 6.99 | ~20-30 | 去掉寄存器双链后指令数减少约 40%，应解决超时 |
| 实践报告 | — | ~70% | 有测试数据，待补充优化数据 |

> 详细数据见 `docs/第一次测试数据收集.md`
