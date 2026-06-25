# 分支合并接口冲突记录

> 记录时间：Day 4  
> 合并分支：`feature/frontend` + `feature/semantic` + `backend` → `main`  
> 状态：已合并，但 B 模块存在接口不兼容，需 B 成员修复后重新推送

## 问题总览

`feature/semantic` 分支未 rebase 到 A 的 Day3 AST 重构之上，导致 4 类接口不匹配。

| 问题 | 严重程度 | 负责人 |
|------|:---:|:---:|
| AST 字段名不匹配 | 高 | B |
| 枚举定义方式变更 | 高 | B |
| Type 方法名变更 | 中 | B |
| SymbolTable 头文件与实现不一致 | 最高 | B |

---

## 问题 1：AST 字段名不匹配

**文件**：`src/semantic/SemanticAnalyzer.cpp`  
**原因**：A 的 Day3 AST 重构后变更了 IfStmt / FuncCall 的字段名

| 旧 API（B 当前使用） | 新 API（A 的 AST 定义） | 出现次数 |
|---|---|---|
| `node->condition` | `node->cond` | 4 |
| `node->thenBranch` | `node->then` | 3 |
| `ifStmt->thenBranch` | `ifStmt->then` | 3 |
| `node->elseBranch` | `node->else_` | 3 |
| `ifStmt->elseBranch` | `ifStmt->else_` | 3 |
| `node->arguments` | `node->args` | 2 |

---

## 问题 2：枚举定义方式变更

**原因**：A 将 `BinaryOp`/`UnaryOp` 从类内嵌枚举改为命名空间级独立枚举，并统一为 PascalCase

### ConstExprEvaluator.cpp

| 旧 API | 新 API |
|--------|--------|
| `using Op = BinaryExpr::Op;` | 删除此行，直接使用 `BinaryOp` |
| `Op::ADD` | `BinaryOp::Add` |
| `Op::SUB` | `BinaryOp::Sub` |
| `Op::MUL` | `BinaryOp::Mul` |
| `Op::DIV` | `BinaryOp::Div` |
| `Op::MOD` | `BinaryOp::Mod` |
| `Op::EQ` | `BinaryOp::Eq` |
| `Op::NE` | `BinaryOp::Ne` |
| `Op::LT` | `BinaryOp::Lt` |
| `Op::LE` | `BinaryOp::Le` |
| `Op::GT` | `BinaryOp::Gt` |
| `Op::GE` | `BinaryOp::Ge` |
| `Op::AND` | `BinaryOp::And` |
| `Op::OR` | `BinaryOp::Or` |
| `UnaryExpr::Op::PLUS` | `UnaryOp::Plus` |
| `UnaryExpr::Op::MINUS` | `UnaryOp::Minus` |
| `UnaryExpr::Op::NOT` | `UnaryOp::Not` |

### SemanticAnalyzer.cpp

| 旧 API | 新 API |
|--------|--------|
| `BinaryExpr::Op::DIV` | `BinaryOp::Div` |
| `BinaryExpr::Op::MOD` | `BinaryOp::Mod` |

---

## 问题 3：Type 方法名变更

**文件**：`src/semantic/SemanticAnalyzer.cpp`  
**原因**：A 的 Type 类方法从 camelCase 改为 snake_case

| 旧 API | 新 API | 次数 |
|--------|--------|:---:|
| `isInt()` | `is_int()` | 4 |
| `isVoid()` | `is_void()` | 1 |

---

## 问题 4：SymbolTable 头文件与实现不一致

**文件**：`src/semantic/SymbolTable.h`、`src/semantic/SymbolTable.cpp`  
**原因**：头文件和实现文件来自两个不同版本

| 差异项 | SymbolTable.h | SymbolTable.cpp |
|--------|--------------|----------------|
| 命名空间 | `namespace semantic` | `namespace toyc` |
| 进入作用域 | `enterScope()` | `pushScope()` |
| 退出作用域 | `exitScope()` | `popScope()` |
| 层级方法 | `currentScopeLevel()` | `currentLevel()` |
| 内部数据结构 | `Scope` 对象栈 | `unordered_map` 直接存储 |
| 额外声明 | `isDefinedInCurrentScope` / `clear` / `reset` | — |
| 缺失声明 | — | `lookupGlobal`（头文件未声明） |

**结果**：.h 和 .cpp 完全无法一起编译。

---

## 责任人分配

| 成员 | 需做的事 | 涉及文件 |
|:---:|---|--|
| **A** | 无需操作。`src/ast/` 是规范来源 | — |
| **B** | 适配新 AST：字段名、枚举值、Type 方法；统一 SymbolTable 命名空间与 API | `SemanticAnalyzer.cpp`、`ConstExprEvaluator.cpp`、`SymbolTable.h`、`SymbolTable.cpp` |
| **C** | 无需操作。backend 独立，无冲突 | — |
| **D** | 等 B 推送修复后重新集成；此文件归档 | — |

## B 成员修复步骤建议

1. `git checkout feature/semantic`
2. `git merge main`（同步最新 main，包含 A 的 AST 重构）
3. 按上表逐项修改 4 个文件
4. 构建验证：`cmake -S . -B build && cmake --build build`
5. 运行语义测试：`tests/test_semantic.cpp`
6. `git push origin feature/semantic`
7. 通知 D 重新合并
