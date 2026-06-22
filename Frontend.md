# ToyC 编译器前端说明

成员 A 负责的前端模块：词法分析、语法分析、AST 构建与调试输出。当前分支 `feature/frontend` 已完成 ToyC 全部语法的解析，输出 AST 供语义分析与代码生成使用。

---

## 1. 模块职责

| 目录 | 职责 |
|------|------|
| `src/lexer/` | 将 ToyC 源码扫描为带位置信息的 Token 序列 |
| `src/parser/` | 递归下降语法分析，构建 AST |
| `src/ast/` | AST 节点定义、访问者接口、调试打印 |
| `src/main.cpp` | 读取 stdin、驱动前端流水线、错误汇总 |

前端**不负责**语义检查、IR 与汇编生成（由成员 B/C 在 `semantic/`、`ir/`、`codegen/` 实现）。

---

## 2. 实现逻辑与数据流

```
stdin (ToyC 源码)
    │
    ▼
read_all_stdin()  →  std::string source
    │
    ▼
Lexer(source)     →  const std::vector<Token>& tokens
    │                  （构造时一次性完成扫描，末尾追加 EOF）
    ▼
Parser(tokens)    →  std::unique_ptr<CompUnit> ast
    │
    ├── TOYC_DUMP_AST=1  →  AstPrinter 输出 AST 到 stderr
    │
    └── 后续：SemanticAnalyzer → CodeGen → stdout（汇编）
```

### 2.1 词法分析（Lexer）

- **扫描方式**：单遍扫描，跳过空白与 `//`、`/* */` 注释。
- **Token 字段**：`type`、`lexeme`、行号、列号；`NUMBER` 额外携带 `int_value`。
- **标识符规则**：`[_A-Za-z][_A-Za-z0-9]*`，与 ToyC 规范一致。
- **整数规则**：`-?(0|[1-9][0-9]*)`；`n-1` 中的 `-` 为二元减号，`-5` 在表达式开头为负数字面量。
- **词法错误**：抛出 `LexError`，由 `main` 输出 `lexical error: ...`。

常见词法错误检测：

| 非法写法 | 报错示例 |
|---------|---------|
| `1a`、`123foo` | `identifier cannot start with a digit` |
| `07` | `leading zeros are not allowed` |
| `foo-bar` | `identifier cannot contain '-'` |
| 单独的 `&` / `\|` | `invalid operator '&'` 等 |
| 未闭合 `/*` | `unterminated block comment` |

### 2.2 语法分析（Parser）

- **方法**：递归下降 + 运算符优先级分层。
- **顶层消歧**：`int x = ...` 为变量声明，`int f(...)` 为函数定义。
- **赋值**：仅作为语句 `ID = Expr;`，**不能**出现在表达式中（如 `(b = 1)` 会报语法错）。
- **表达式优先级**（从低到高）：`||` → `&&` → 关系 → `+/-` → `*/%` → 一元 →  primary。
- **短路保留**：`&&` / `||` 保留为 `BinaryExpr`，不在前端做常量折叠，便于后端实现短路。
- **语法错误**：抛出 `ParseError`，由 `main` 输出 `parse error: ...`。

### 2.3 AST 设计

- 所有节点继承 `AstNode`，含 `kind()`、`location()`、`accept(AstVisitor&)`。
- 子节点用 `std::unique_ptr` 持有；顶层 `CompUnit::items()` 为 `variant<VarDecl, ConstDecl, FuncDef>`。
- `AstVisitor` 为每种节点定义 `visit()`；`AstPrinter` 是调试与后续模块的参考实现。

**AST 节点一览：**

```
CompUnit
├── VarDecl / ConstDecl / FuncDef
│   └── BlockStmt → Stmt*
│       ├── EmptyStmt / ExprStmt / AssignStmt / DeclStmt
│       ├── IfStmt / WhileStmt
│       ├── BreakStmt / ContinueStmt / ReturnStmt
│       └── BlockStmt（嵌套）
└── Expr 子树
    ├── IntLiteral / IdentifierExpr / CallExpr
    ├── UnaryExpr / BinaryExpr（含 And / Or）
    └── ...
```

---

## 3. 目录结构

```text
src/
├── main.cpp              # 编译器入口
├── lexer/
│   ├── Token.h / Token.cpp
│   ├── Lexer.h / Lexer.cpp
│   └── LexError.h        # 词法错误
├── parser/
│   └── Parser.h / Parser.cpp   # 含 ParseError
└── ast/
    ├── AstNode.h         # 基类、SourceLocation、AstNodeKind
    ├── AstVisitor.h      # 访问者接口（语义/IR/CodeGen 扩展点）
    ├── CompUnit.h        # 编译单元根
    ├── Decl.h            # VarDecl、ConstDecl
    ├── FuncDef.h         # FuncDef、Param
    ├── Stmt.h            # 各类语句
    ├── Expr.h            # 各类表达式
    └── AstPrinter.h / AstPrinter.cpp
```

---

## 4. 构建与运行

### 4.1 依赖

- C++20
- CMake ≥ 3.16
- 编译器：MSVC（Visual Studio 2022）或 MinGW（CLion 自带）

### 4.2 构建

**方式 A：CLion / MinGW（`cmake-build-debug/`）**

```powershell
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug
```

可执行文件：`cmake-build-debug/toyc.exe`

> MinGW 运行时不在系统 PATH 时，运行前需添加：
> `$env:Path = "your-MinGW-bin-PATH;" + $env:Path`

**方式 B：Visual Studio / MSVC（`build/`）**

```powershell
& "your-VS-cmake.exe-PATH" -S . -B build
& "yout-VS-cmake.exe-PATH" --build build --config Debug
```

可执行文件：`build\Debug\toyc.exe`

### 4.3 运行

编译器从 **stdin** 读源码，当前前端阶段向 **stdout** 输出 `// ToyC frontend: parse succeeded`（完整编译器应输出汇编）。

PowerShell 不支持 `<` 重定向，请用管道：

```powershell
# 基本运行
Get-Content tests\basic\return_7.tc | .\cmake-build-debug\toyc.exe

# 查看 AST
$env:TOYC_DUMP_AST = "1"
Get-Content tests\basic\return_7.tc | .\cmake-build-debug\toyc.exe 2>&1

# 清除 AST 输出
Remove-Item Env:TOYC_DUMP_AST -ErrorAction SilentlyContinue
```

Linux / 评测环境：

```bash
./build/toyc < tests/basic/return_7.tc
./build/toyc -opt < tests/basic/return_7.tc   # 完整编译器支持 -opt
```

### 4.4 清理构建目录

`build/` 与 `cmake-build-debug/` 均为 CMake 生成物，可安全删除后重新配置构建。

---

## 5. 测试用例

### 5.1 `tests/basic/` — 基础功能样例

| 文件 | 测试点 | 预期（完整编译器） | 前端阶段 |
|------|--------|-------------------|---------|
| `return_0.tc` | 最简 `return 0` | 退出码 0 | 解析成功 |
| `return_7.tc` | 表达式优先级 `1+2*3` | 退出码 7 | 解析成功 |
| `return_1.tc` | 局部变量与算术 | 退出码 7 | 解析成功 |
| `return_2.tc` | 全局变量 | 退出码 10 | 解析成功 |
| `return_3.tc` | 全局常量 | 退出码 101 | 解析成功 |
| `return_4.tc` | 短路求值 `\|\|` | 退出码 0（`b` 未被修改） | 解析成功 |
| `return_5.tc` | 递归、`fib(n-1)` 实参 | 退出码 55 | 解析成功 |
| `return_6.tc` | `while` + `break`/`continue` | 退出码 25 | 解析成功 |

**`return_4.tc` 要点**：用 `side_effect()` 函数副作用代替 C 风格的 `(b = 1)`，因 ToyC 不允许赋值表达式。

**`return_6.tc` 要点**：ToyC 合法形式（`while` 循环、分开声明、`s = s + i`）。ToyC **不支持** `for`、`+=`、逗号声明。

### 5.2 `tests/basic/` — 错误样例

| 文件 | 测试点 | 预期 |
|------|--------|------|
| `return_f0.tc` | 缺少 `{` 的语法错误 | `parse error: ...` |
| `return_f1.tc` | 标识符以数字开头 `1a` | `lexical error: identifier cannot start with a digit` |
| `return_f6.tc` | C 语法：`int i,s=0`、`for`、`+=` | `parse error: ...`（非 ToyC 语法） |

### 5.3 `tests/parser/full_syntax.tc`

覆盖全局常量/变量、多函数、`void` 函数、空语句、`if/else`、`while`、`break`/`continue`、函数调用与短路逻辑的综合性语法样例，前端应完整解析为 AST。

### 5.4 其他测试目录

| 目录 | 用途 |
|------|------|
| `tests/lexer/` | 词法单元测试（关键字、标识符、注释等） |
| `tests/semantic/` | 语义分析测试（成员 B） |
| `tests/control_flow/` | 控制流端到端测试 |
| `tests/performance/` | 性能测试 |
| `scripts/run_tests.ps1` | 端到端测试脚本（成员 D 维护） |

---

## 6. 对外接口

### 6.1 最小调用方式

```cpp
#include "lexer/Lexer.h"
#include "parser/Parser.h"

const std::string source = /* 从 stdin 或文件读取 */;
Lexer lexer(source);
Parser parser(lexer.tokens());
std::unique_ptr<CompUnit> ast = parser.parse();
// ast 即为前端交付物
```

### 6.2 错误类型

| 类型 | 触发阶段 | 输出前缀 | 位置信息 |
|------|---------|---------|---------|
| `LexError` | 词法分析 | `lexical error:` | `error.location()` |
| `ParseError` | 语法分析 | `parse error:` | `error.location()` |

```cpp
// main.cpp 中的捕获顺序
catch (const LexError& error)   { /* stderr: lexical error: ... */ }
catch (const ParseError& error) { /* stderr: parse error: ... */ }
```

### 6.3 AstVisitor 集成（成员 B / C）

语义分析与代码生成应实现 `AstVisitor` 子类，在 `parser.parse()` 之后遍历 AST：

```cpp
// 语义分析（成员 B）
SemanticAnalyzer analyzer;
analyzer.analyze(*ast);

// 代码生成（成员 C）
CodeGen codegen(std::cout, optimize);
codegen.emit(*ast);
```

参考实现：`AstPrinter`（`src/ast/AstPrinter.h`）。

**集成约定：**

- 输入：始终为完整源码字符串。
- 输出：`unique_ptr<CompUnit>`，所有权在调用方。
- 不修改 AST；语义信息放入符号表或 IR 层。
- 所有 AST 节点均含 `SourceLocation`，错误报告应复用行列格式。

### 6.4 调试开关

| 环境变量 | 作用 |
|---------|------|
| `TOYC_DUMP_AST=1` | 将 AST 树形结构打印到 stderr |

---

## 7. ToyC 语法覆盖说明

前端已覆盖 `任务要求.md` 中的文法，主要限制（与 C 的差异）：

| 特性 | ToyC | 前端处理 |
|------|------|---------|
| 变量声明 | `int ID = Expr;`（单个） | ✅ |
| 逗号声明 `int a, b = 0` | ❌ | 报语法错 |
| 赋值表达式 `(b = 1)` | ❌ | 报语法错 |
| `for` 循环 | ❌ | 报语法错 |
| 复合赋值 `+=` | ❌ | 报语法错 |
| `while` / `if` / 递归 / 短路 | ✅ | ✅ |

---

## 8. 后续工作

- [ ] 成员 B：在 `main.cpp` 的 `parser.parse()` 后接入 `SemanticAnalyzer`
- [ ] 成员 C：语义通过后接入 `CodeGen`，stdout 输出 RISC-V32 汇编
- [ ] 成员 D：完善 `run_tests.ps1`，对比 ToyC 与 GCC 退出码
- [ ] 可选：补充 `tests/lexer/`、`tests/parser/` 自动化脚本

---

## 9. 相关文档

- [任务要求.md](./任务要求.md) — ToyC 语言定义与评测规范
- [开发任务与分工.md](./开发任务与分工.md) — 四人分工与里程碑
- [README.md](./README.md) — 项目总览
