# Day3 更新说明 — 前端 AST 字段详解与调用指南

本文档记录 **Day3 前端 AST 按 [Semantic.md](./Semantic.md) 对齐** 后的字段约定、与 B 端现有实现的差异，以及 B/C/D 的调用方式。前端实现细节见 [FrontEnd.md](./FrontEnd.md)。

**核心原则**：AST **唯一**定义在 `src/ast/`（`namespace toyc`）；B 侧应 `#include "ast/AST.h"` 并实现 `toyc::ASTVisitor`，**删除**独立的 `semantic/AST.h`。

---

## 1. Day3 前端变更摘要

| 变更项 | 旧前端 | Day3 当前实现 |
|--------|--------|---------------|
| AST 头文件 | 分散（`AstNode.h`、`Expr.h` 等） | `ASTBase.h` + `AST.h` |
| 命名空间 | 无 | `namespace toyc` |
| 字段访问 | getter（`line()`、`is_global()`） | 公开字段（`line`、`isGlobal`） |
| 访问者 | `AstVisitor` | `ASTVisitor` |
| 编译单元 | `CompUnit::items()` | `CompUnit::elements` |
| 语句块 | `BlockStmt::stmts()` | `Block::statements` |
| 字面量/调用 | `IntLiteral`、`CallExpr` | `Number`、`FuncCall` |
| 块内声明 | `DeclStmt` 包装 | `VarDecl`/`ConstDecl` 直接进入 `Block::statements` |
| 空语句 `;` | `EmptyStmt` | 不生成节点 |
| If/While 单子句 | 直接挂 `ReturnStmt` 等 | **自动包一层 `Block`**（见 §3.4） |

---

## 2. Semantic.md 必需字段 — 三方对照

下表对比 [Semantic.md](./Semantic.md) 要求、**当前前端（A）** 实现、以及 **B 端现有代码**（`ToyC-Compiler-feature-semantic/src/semantic/`）的实际用法。

| 字段 / 约定 | Semantic.md | 前端 A（`src/ast/`） | B 当前代码 | 对齐状态 |
|-------------|-------------|---------------------|------------|:--------:|
| 头文件 | `ast/AST.h` | ✅ `ast/AST.h` | `semantic/AST.h` | B 需改 |
| 命名空间 | `toyc`（示例） | ✅ `toyc` | `semantic::ast` | B 需改 |
| 遍历方式 | `ASTVisitor` | ✅ `accept` + `ASTVisitor` | `dynamic_cast` + 手写 `visitXxx` | B 需改 |
| `ASTNode::line` / `column` | `int` 公开字段 | ✅ | `int line`（Stmt 无 column） | 基本对齐 |
| `Expr::type` | `Type*` | ✅；字面量/运算为 `int`，`Identifier`/`FuncCall` 为 `nullptr` | 自有 `Type`（`isInt`/`isVoid`） | B 需改用 `toyc::Type` |
| `VarDecl::initExpr` | 必需 | ✅ | ✅ `initExpr` | 对齐 |
| `ConstDecl::initExpr` | 必需 | ✅ | ✅ `initExpr` | 对齐 |
| `isGlobal` | `VarDecl`/`ConstDecl` | ✅ 节点字段 | 由 `visitVarDecl(..., isGlobal)` 参数传入，**节点无字段** | B 需读 `node.isGlobal` |
| `FuncDef::returnType` | `Type*` | ✅ | ✅；判 void 用 `returnType->isVoid` | B 需改 `is_void()` |
| `FuncCall::isStatement` | `bool` | ✅ Parser 填充 | ✅ `isStatement` | 对齐（字段名一致） |
| `AssignStmt::lvalue` | `ID*` / `Identifier*` | ✅ `unique_ptr<Identifier>` | `Assignment::lvalue` 为 `Expr*` | B 需改类名与类型 |
| `CompUnit` 顶层 | `elements` | ✅ `vector<TopLevelElement>` | `globalDecls` + `functions` 分拆 | B 需改 |
| `Block` 语句列表 | `statements` | ✅ | `Block::stmts` | B 需改 |
| `FuncCall` 名称/实参 | `name`、`args` | ✅ | `calleeName`、`arguments` | B 需改 |
| `ReturnStmt` 表达式 | `value` | ✅；`nullptr` 表示 `return;` | `returnExpr` | B 需改 |
| `IfStmt` 条件 | `cond` | ✅ `cond` | `condition` | B 需改 |
| `IfStmt` 分支 | `then`、`else`（`Stmt`） | ✅ `then`、`else_`（`Stmt*`） | `thenBranch`、`elseBranch`（`Block*`） | 见 §3.4 |
| `WhileStmt` | `cond`、`body` | ✅ | `condition`、`body`（当作 `Block*`） | 见 §3.4 |
| 二元/一元运算 | `BinaryExpr`/`UnaryExpr` + `Op` | ✅ 枚举 `BinaryOp`/`UnaryOp` | `BinaryOp`/`UnaryOp` 结构体 + `string op` | B 需改 |
| 函数形参 | `Param` | ✅ `Param` | `vector<VarDecl*>` | B 需改 |
| `VarDecl::isConst` | 无（常量在 `ConstDecl`） | ❌ 无此字段 | B 错误读取 `decl->isConst` | B 逻辑错误，勿由 A 添加 |

**结论**：Semantic.md 要求的 P0/P1 字段，**前端 A 已全部提供**；B 尚未切换到 `ast/AST.h`，字段名与结构仍按旧 `semantic/AST.h` 编写。

---

## 3. 前端 AST 字段详解（当前实现）

头文件：`#include "ast/AST.h"`，`using namespace toyc;`

### 3.1 基类与类型

```cpp
// ASTBase.h — 全局
struct SourceLocation { int line; int column; };

// AST.h — namespace toyc
class ASTNode { public: int line, column; virtual void accept(ASTVisitor&) const = 0; };
class Expr : public ASTNode { public: Type* type; };
class Stmt : public ASTNode { /* ... */ };

// Type.h
Type::intType();   // int
Type::voidType();  // void
// 判断：returnType->is_void() / is_int()
```

### 3.2 必需字段（Semantic.md P0/P1）

| 节点 | 字段 | 类型 | Parser 填充规则 |
|------|------|------|----------------|
| 所有节点 | `line`, `column` | `int` | 来自 Token 位置 |
| `Expr` | `type` | `Type*` | `Number`/`BinaryExpr`/`UnaryExpr` → `intType()`；`Identifier`/`FuncCall` → `nullptr`（语义阶段补全） |
| `VarDecl` | `name`, `initExpr`, `isGlobal` | `string`, `unique_ptr<Expr>`, `bool` | 顶层 `true`，块内 `false` |
| `ConstDecl` | 同上 | 同上 | 同上 |
| `FuncDef` | `returnType`, `name`, `params`, `body` | `Type*`, `string`, `vector<Param>`, `unique_ptr<Block>` | — |
| `FuncCall` | `name`, `args`, `isStatement` | `string`, `vector<Expr>`, `bool` | 仅 `ExprStmt` 形式为 `true` |
| `AssignStmt` | `lvalue`, `value` | `unique_ptr<Identifier>`, `unique_ptr<Expr>` | 独立 `Identifier` 节点 |
| `CompUnit` | `elements` | `vector<variant<VarDecl, ConstDecl, FuncDef>>` | — |
| `Block` | `statements` | `vector<unique_ptr<Stmt>>` | — |

### 3.3 节点一览与构造

| 节点 | 主要字段 | 说明 |
|------|---------|------|
| `Number` | `value` | 整数字面量 |
| `Identifier` / `ID` | `name` | 标识符 |
| `BinaryExpr` | `left`, `op`, `right` | `BinaryOp` 枚举 |
| `UnaryExpr` | `op`, `operand` | `UnaryOp` 枚举 |
| `FuncCall` | `name`, `args`, `isStatement` | 函数调用 |
| `ExprStmt` | `expr` | 表达式语句 |
| `ReturnStmt` | `value` | 可为 `nullptr`（`return;`） |
| `IfStmt` | `cond`, `then`, `else_` | `else_` 可为 `nullptr` |
| `WhileStmt` | `cond`, `body` | — |
| `BreakStmt` / `ContinueStmt` | — | 仅位置信息 |
| `Param` | `name` | 形参 |

**前端不再提供的节点**：`DeclStmt`、`EmptyStmt`、`IntLiteral`、`CallExpr`、`BlockStmt` 等旧名。

### 3.4 If / While 单子句自动包 `Block`（Day3 新增）

为兼容 B 端按 `Block` 遍历分支的习惯，Parser 对 **非 `{ ... }` 的单语句分支** 自动包装：

```c
if (x) return 0;     // then → Block { ReturnStmt }
while (x) break;     // body → Block { BreakStmt }
if (x) { ... }       // 已是 Block，不重复包装
```

- 字段类型仍为 `Stmt*`（Semantic.md 约定）
- 单子句时 **动态类型为 `Block`**，B 可 `visit(Block)` 或 `dynamic_cast<Block*>(node.then.get())`
- 推荐 B 实现：`visit(IfStmt)` 内对 `then`/`else_` 调用 `stmt->accept(*this)`，而非假定一定是 `Block*`

### 3.5 `FuncCall::isStatement` 填充规则

| 源码上下文 | `isStatement` |
|------------|:-------------:|
| `foo();`（`ExprStmt`） | `true` |
| `return foo();`、`x = f();`、实参、`if (f())` 等 | `false` |

### 3.6 `isGlobal` 填充规则

| 位置 | `isGlobal` |
|------|:----------:|
| 顶层 `int g = 1;` | `true` |
| 函数体内 `int x = 1;` | `false` |

---

## 4. B 端迁移对照（旧代码 → 当前前端）

B 合并代码时，按下列映射修改 `SemanticAnalyzer.cpp`、`ConstExprEvaluator.cpp` 等：

| B 旧写法 | 改为（前端 AST） |
|----------|------------------|
| `#include "semantic/AST.h"` | `#include "ast/AST.h"` |
| `semantic::ast::` | `toyc::` |
| `node->getGlobalDecls()` / `getFunctions()` | `for (item : cu.elements) std::visit(...)` |
| `block->stmts` | `block->statements` |
| `call->calleeName` | `call->name` |
| `call->arguments` | `call->args` |
| `Assignment` | `AssignStmt` |
| `assign->rvalue` | `assign->value` |
| `ret->returnExpr` | `ret->value` |
| `ifStmt->condition` | `ifStmt->cond` |
| `ifStmt->thenBranch` / `elseBranch` | `ifStmt->then` / `else_` |
| `whileStmt->condition` | `whileStmt->cond` |
| `returnType->isVoid` | `returnType->is_void()` |
| `BinaryOp`/`UnaryOp`（`string op`） | `BinaryExpr`/`UnaryExpr` + `binary_op_name(op)` |
| `func->params` 为 `VarDecl*` | `func->params` 为 `vector<unique_ptr<Param>>` |
| `decl->isConst`（VarDecl 上） | 删除；常量只看 `ConstDecl` + 符号表 |
| `dynamic_cast` 遍历 | 继承 `ASTVisitor`，实现 17 个 `visit` |
| `analyze()` 返回 `hasErrors()` | 与 `main` 统一约定（建议 `true`=通过） |

---

## 5. 各方调用方式

### 5.1 Role B — 语义分析

**依赖头文件：**

```cpp
#include "ast/AST.h"
#include "semantic/SemanticAnalyzer.h"
```

**流水线：**

```cpp
using namespace toyc;

Lexer lexer(source);
Parser parser(lexer.tokens());
std::unique_ptr<CompUnit> ast = parser.parse();
// 先处理 lex / parse 错误

SemanticAnalyzer analyzer;
if (!analyzer.analyze(ast.get())) {  // 约定：false = 有错误（集成时与 B 头文件对齐）
    for (const SemanticError& err : analyzer.errors()) {
        std::cerr << err.line << ':' << err.column
                  << ": error: [SEM_xxx] " << err.message << '\n';
    }
    return 1;
}
```

**`SemanticAnalyzer` 应继承 `ASTVisitor`：**

```cpp
class SemanticAnalyzer : public toyc::ASTVisitor {
    bool analyze(toyc::CompUnit* root);
    const std::vector<SemanticError>& errors() const;
    Symbol* lookupGlobal(const std::string& name);
    Symbol* lookupLocal(const std::string& name);
    bool getConstantValue(const std::string& name, int* out_value);
    // 实现全部 visit(const X&) override ...
};
```

**典型 `visit` 实现：**

```cpp
void SemanticAnalyzer::visit(const CompUnit& node) {
    for (const auto& item : node.elements) {
        std::visit([this](const auto& p) { p->accept(*this); }, item);
    }
}

void SemanticAnalyzer::visit(const Block& node) {
    for (const auto& stmt : node.statements) {
        stmt->accept(*this);
    }
}

void SemanticAnalyzer::visit(const IfStmt& node) {
    node.cond->accept(*this);
    node.then->accept(*this);           // 单子句时内部为 Block
    if (node.else_ != nullptr) {
        node.else_->accept(*this);
    }
}

void SemanticAnalyzer::visit(const ReturnStmt& node) {
    if (node.value != nullptr) {
        node.value->accept(*this);
    }
}
```

**语义检查依赖的前端字段：**

| 错误类型 | 依赖字段 |
|----------|---------|
| 未声明标识符 | `Identifier::name` |
| 重复定义 | `isGlobal`、作用域栈 |
| 常量赋值 | `AssignStmt::lvalue` |
| void 调用上下文 | `FuncCall::isStatement` |
| 参数个数 | `FuncCall::args` |
| main 检查 | `FuncDef::name`、`returnType` |
| 常量初始化 | `initExpr` + `ConstExprEvaluator` |

### 5.2 Role C — 代码生成

**在语义分析通过后**使用同一 AST 与符号表：

```cpp
#include "ast/AST.h"
#include "semantic/SemanticAnalyzer.h"

// analyzer.analyze(ast) 已通过

Symbol* sym = analyzer.lookupGlobal("x");
if (sym) {
    sym->kind;       // GLOBAL_VAR / LOCAL_VAR / FUNCTION ...
    sym->isConst;
    sym->constValue; // optional<int>
    sym->offset;     // 初始 -1，C 栈布局时写入
    sym->isGlobal;
    sym->isUsed;
}

Symbol* local = analyzer.lookupLocal("i");

// 遍历 AST：实现 ASTVisitor 子类，或参考 AstPrinter
AstPrinter printer(std::cerr);
printer.print(*ast);
```

C 应通过 `ASTVisitor` 遍历 `toyc::` AST，**不要** include `semantic/AST.h`。

### 5.3 Role D — 优化器

```cpp
// 常量传播
int val;
if (analyzer.getConstantValue("N", &val)) {
    // 用 val 替换引用
}

// 死代码消除
Symbol* sym = analyzer.lookupGlobal("unused");
if (sym && !sym->isUsed) { /* 可移除 */ }

// 表达式折叠可复用 ConstExprEvaluator（入参需改为 toyc::Expr*）
```

### 5.4 调试 AST

```powershell
$env:TOYC_DUMP_AST = "1"
Get-Content tests\parser\full_syntax.tc | .\cmake-build-debug\toyc.exe 2>&1
```

AstPrinter 输出标记：`VarDecl x global/local`、`FuncCall foo stmt`、`Block` 嵌套等。

---

## 6. 编译流水线

```
源代码 (.tc)
  → Lexer + Parser（A）
  → toyc::CompUnit（字段已填充，见 §3）
  → SemanticAnalyzer::analyze（B）
  → 符号表 + 已验证 AST
  → CodeGen（C，填充 Symbol::offset）
  → Optimizer（D，可选）
  → RISC-V 汇编
```

**边界**：语义失败则不进入 C/D；词法/语法失败则不进入语义。

---

## 7. 集成状态检查清单

| 项 | 负责 | 状态 |
|----|------|:----:|
| AST 按 Semantic.md 实现（`src/ast/AST.h`） | A | ✅ |
| P0/P1 字段（§3.2） | A | ✅ |
| If/While 单子句包 `Block` | A | ✅ |
| B 删除 `semantic/AST.h`，改用 `ast/AST.h` | B | 待做 |
| B 实现 `toyc::ASTVisitor` | B | 待做 |
| B 按 §4 迁移字段名 | B | 待做 |
| `main.cpp` 串联 + CMake 合并 | A+B | 待做 |
| 跑通 `tests/semantic/` | A+B | 待做 |
| C 使用 `lookupGlobal/Local`、`Symbol::offset` | C | 后续 |
| D 使用 `getConstantValue` | D | 后续 |

---

## 8. 相关文档

- [Semantic.md](./Semantic.md) — 语义模块权威接口（A 交付 AST 的依据）
- [FrontEnd.md](./FrontEnd.md) — 词法/语法/Parser 规则
- [开发任务与分工.md](./开发任务与分工.md) — 四人分工
