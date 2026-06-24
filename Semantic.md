# ToyC Compiler — 语义分析模块 (Role B)

ToyC 语言编译器的语义分析中间模块，负责验证前端生成的 AST 是否符合语言语义约束，并为后端代码生成提供符号信息。

## 项目结构

```
├── CMakeLists.txt              # CMake 构建配置
├── docs/
│   └── semantic-design.md      # 语义分析设计文档
├── src/
│   ├── main.cpp                # 主入口（桩程序 / 集成入口）
│   ├── ast/
│   │   ├── ASTBase.h           # AST 基类（ASTNode, Expr, Stmt）
│   │   ├──.h               # AST 节点定义 + ASTVisitor 接口
│   │   └── Type.h              # 类型系统（int / void）
│   └ semantic/
│       ├── Symbol.h            # 符号表定义
│ ├── Symbol.cpp
│       ├── SymbolTable.h       # 符号表（嵌套作用域）
│       ├── SymbolTable.cpp
│       ├── ConstEvaluator.h # 常量表达式求值器
│       ├── ConstExprEvaluator.cpp
│       ├── SemanticAnalyzer.h   # 语义分析器
│       ├── SemanticAnalyzer.cpp
│       └── errors.h            # 错误码与错误信息
├── tests/
│   ├ semantic/
│   │   ├── valid/              # 16 个合法 ToyC 源文件测试用例
│   │   └── invalid/            # 16 个非法 ToyC 源文件测试用例
│   └── test_simple.cpp         # 基础测试
└── build/                      # 构建产物
```

## 构建

### 依赖

- C++20 编译器（g++ 16+ / clang 14+ / MSVC 2022+）
- CMake 3.20+（可选）

### 使用 CMake

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### 使用 g++ 直接编译

```bash
g++ -std=c++20 -I src -c src/semantic/Symbol.cpp -o build/Symbol.o
g++ -std=c++20 -I src -c src/semantic/SymbolTable.cpp -o build/SymbolTable.o
g++ -std=c++20 -I src -c src/santic/ConstExprEvaluator.cpp -o build/ConstExprEvaluator.o
g++ -std=c++20 -I src -c src/semantic/SemanticAnalyzer.cpp -o build/SemanticAnalyzer.o
g++ -std=c++20 -I src -c src/main.cpp -o build/main.o
g++ -std=c++20 -o build/toyc.exe build/*.o
```

## 运行

```bash
echo "int main(void) { return 0; }" | build/toyc.exe```

## 测试

```bash
g++ -std=c++20 -I src tests/test_simple.cpp build/*.o -o build_simple.exe
./build/test_simple.exe
```

## 核心功能

- **符号表管理** — 支持嵌套作用域，提供 pushScope/popScope/insert/lookup 等操作
- **常量表达式求值** — 编译期折叠算术、比较、逻辑运算，支持短路求值
- **语义检查** — 覆盖 15 类语义错误检测（见下表）

|错误码 | 说明 |
|--------|------|
| SEM_001 | 未声明的标识符 |
| SEM_002 |重复定义 |
| SEM_003 | 对常量 |
| SEM_004 | 修改常量值 |
| SEM_005 | 除零错误 |
| SEM_006 | main 函数错误 |
| SEM_007 | 函数参数数量匹配 |
| SEM_008 | break 在循环外 |
| SEM_009 | continue 在循环外 |
| SEM_010 | 无效的赋值目标 |
| SEM_011 | void 表达式用于值上下文 |
| SEM_012 | 非 void 函数缺少返回语句 |
| SEM013 | 返回路径不完整 |
| SEM_014 | 类型不匹配 |
| SEM_015 | 编译期常量错误 |

## 对外接口规范

 Role A（前端/语法分析器）→ 调用本模块

完成词法分析和语法分析后，构建 ToyC AST，然后 SemanticAnalyzer 进行检查。

**构建 AST 的节点类型（定义在 src/ast/AST.h）：**

| 节点 | 构造参数 | 说明 |
|------|----------|------|
| CompUnit | () | 根节点，elements 向量存放所有顶层声明 |
| FuncDef | (Type* retType, string name, vector<Param>, Block body) | 函数定义 |
| VarDecl | (string name, Expr init, bool isGlobal) | 变量声明 |
| ConstDecl | (string name, Expr init, bool isGlobal) | 常量声明 |
| Block | () | 复合语句块 |
| ReturnStmt | (Expr value) | 返回语句 |
| IfStmt | (Expr cond, Stmt then, Stmt else) | if 语句 |
| WhileStmt | (Expr cond, Stmt body) | while 循环 |
| BreakStmt | () break 语句 |
| ContinueStmt | () | 语句 |
| AssignStmt | (string name, Expr value) | 赋值语句 |
| ExprStmt | ( expr) | 表达式语句 |
| Number | (int value) | 整数字面量 |
| Identifier | (string name) | 标识符引用 |
| BinaryExpr | (Expr left, Op op, Expr right) | 二元运算表达式 |
| UnaryExpr | (Op op, Expr operand) | 一元运算表达式 |
| FuncCall | (string name, vector<Expr> args) | 函数调用 |
| Param | (string name) | 函数形参 |

### A 必须提供的 AST 字段

| 节点类型           | 字段        | 类型  | 用途                           | 必要性 |
| ------------------ | ----------- | ----- | ------------------------------ | ------ |
| ASTNode（基类）    | line        | int   | 定位                           | 必需   |
| ASTNode（基类）    | column      | int   | 错误定位                       | 必需   |
| Expr（表达式基类） | type        | Type* | 类型一致性检查、void 判断      | 必需   |
| VarDecl            | initExpr    | Expr* | 变量初始化值                   | 必需   |
| ConstDecl          | initExpr    | Expr* | 常量初始化值                   | 必需   |
| Decl/ConstDecl     | isGlobal    | bool  | 区分全局/局部作用域            | 必需   |
| FuncDef            | returnType  | Type* | 函数返回类型检查               | 必需   |
| FuncCall           | isStatement | bool  | 判断 void 函数调用是否仅作语句 | 必需   |
| AssignStmt         | lvalue      | ID*   | 左值标识符（检查 const 赋值）  | 必需   |

特别提醒 A：Expr::type 和 FuncCall::isStatement 目前 AST.h 中尚未定义，A 需要在节点定义中补充这两个字段。

类型使用 Type::intType() / Type::voidType() 获取（定义在 src/ast/Type.h）。

**调用示例**：

```cpp
#include "semantic/SemanticAnalyzer.h"

using namespace toyc;

// 1. 构建 AST
auto body = make_unique<Block>();
    body->statements.push_back(
    make_unique<ReturnStmt>(make_unique<Number>(0)));

auto func = make_unique<FuncDef>(
    Type::intType(), "main",
    vector<unique_ptr<Param>>(), move(body));

auto cu = make_unique<CompUnit>();
cu->elements.push_backove(func));

// 2. 语义分析
SemanticAnalyzer analyzer;
bool ok = analyzer.analyze(cu.get());

if (ok) {
    // 语义检查通过
} else {
    for (const auto& err : analyzer.errors()) {
        cerr << err.line << ":" << err.column << ": "
             << err.message << endl;
   }
```

---

### 完整编译流水线（A → B → C → D）

```
源代码 (.tc)
    ↓
[Role A: 词法分析 + 语法分析]
    ↓  构建 AST（必须填充 line/column/type/isStatement 等字段）
AST 节点树
    ↓  调用 SemanticAnalyzer::analyze(root)
[Role B: 语义分析] ── 报错则终止编译
    ↓
已验证的 AST + SymbolTable
    ↓  lookupGlobal() / lookupLocal() / getConstantValue()
[Role C 后端代码生成]
    ↓  填充 Symbol::offset（栈帧偏移）
RISC-V 汇编 / IR
    ↓  getConstantValue() / isUsed / ConstExprEvaluator
[Role D: 优化器]
    ↓
优化后的目标代码
```

**入口调用时序（多角色代码合并参考）：**

```cpp
// main.cpp — 完整编译入口

#include "ast/AST.h"
#include "semantic/SemanticAnalyzer.h"
// ... 其他角色头文件

int main(int argc, char* argv[]) {
    // [A] 词法分析 + 语法分析
    // std::unique_ptr<CompUnit> ast = Parser::parse(std::cin);

    // [B] 语义分析    SemanticAnalyzer analyzer;
    if (!analyzer.analyze(ast.get())) {
        for (const auto& err : analyzer.errors()) {
            std::cerr << err.line << ":" << err.column
                      << ": error: << static_cast<int>(err.code)
                      << "] " << err.message << std::endl;
               return 1;  // 语义错误，终止编译
    }

    // [C] 后端代码生成
    // CodeGenerator generator(ast.get(), analyzer);
    // generator.generate();

    // [D] 优化（可选）
    // Optimizer optimizer(ast.get(), analyzer);
    // optimizer.optimize();

    return 0;
}
```

---

### Role C（后端/代码生成器）→ 查询本模块结果

语义分析通过后，Role C 可以通过 SemanticAnalyzer 查询符号信息：

```cpp
// analysis 通过后查询
Symbol* sym = analyzer.lookupGlobal("x");
if (sym {
    sym->kind          // GLOBAL_VAR / LOCALAR / FUNCTION 等
    sym->isConst;       // 是否常量
    sym->constValue;    // 编译期常量值（optional<int>）
    sym->isUsed;        // 是否被使用过（供死代码消除）
    sym->paramCount;    // 参数个数（仅 FUNCTION 类型有效）
    sym->returnsInt; // 是否返回 int
}

// 只查询当前作用域
Symbol local = analyzer.lookupLocal("i");

// 查询编译期常量值
int val;
if (analyzer.getConstantValueN", &val)) {
    // val 即为常量 N 的值
}
**Symbol 结构体字段（定义在 src/semantic/Symbol.h）：**

| 字段 | 类型 | 说明 |
|------|------|------|
| name | string | 符号名称 |
| kind | SymbolKind | 符号类型枚举 |
| isConst | bool | 是否为常量| constValue | optional<int> | 编译期值（如有） |
| line | int | 声明所在行号 |
| offset | int | 符号偏移量（供后端使用） |
| isGlobal | bool | 是否全局符号 |
| isUsed | bool | 是否被引用（供 DCE 使用） |
| paramCount | int | 函数参数数量 |
| returnsInt | bool | 函数返回类型是否为 int |

**SymbolKind 枚举：**

| 值 | 说明 |
|----|------|
| GLOBAL_VAR | 全局变量 |
| GLOBALST | 全局常量| LOCAL_VAR | 局部变量 |
| LOCAL_CONST | 局部常量 |
| PARAM | 函数形参 |
| FUNCTION | 函数定义 |

**Symbol::offset 约定（C 需要填充）：**

语义分析阶段将 Symbol::offset 初始化为 -1。C 在代码生成阶段进行栈帧布局时，需要为每个局部变量和形参分配偏移量并写入该字段：

```cpp
// C 在栈分配时填充 offset
Symbol* localI = analyzer.lookupLocal("i");
if (localI && localI->offset == -1) {
    localI->offset = -8;   // 例如分配到 rbp-8
}

// 生成指令时读取 offset
int memOffset = sym->offset;  // -1 表示尚未分配
```

---

### Role D（优化器）→查询本模块结果

优化器可以复用语义分析阶段收集的信息：

```cpp
// 常量传播 — 查询任意标识符的编译期常量值
int val;
if (analyzer.getConstantValue("N", &val)) {
    // 可以用 val 替换使用 N 的位置
}

// 死代码消除 — 检查符号是否被使用
Symbol* sym = analyzer.lookupGlobal("unusedVar");
if (sym && !sym->isUsed) {
    // 可以安全移除该全局变量
}

// 复用常量求值器进行表达式折叠
ConstExprEvaluator evaluator(symTable);
auto result = evaluator.evaluate(someExpr);  // optional<int>
```

###错误信息格式

分析失败时，通过 errors() 获取错误，每个 SemanticError 包含：

```cpp
struct SemanticError {
    int line;               // 错误行号
    int column;             // 错误列号
    SemanticErrorCode code; // 错误码（15 种，见上表）
    string message;         // 可读的错误描述
```

输出到 stderr 的格式约定：

```
行:列: error: [SEM_XXX] 错误描述
```

---

## 多角色集成规范

### 1. 项目目录合并规则

四个角色的代码在集成时，按以下目录结构合并：

```
├── CMakeLists.txt              合并后的 CMake 构建（包含所有角色源文件）
├── src/
│   ├── main.cpp                # 主入口（A → B → C → D 串联）
│   ├── ast/                    # Role A：AST 节点定义（已有骨架）
│   ├── lexer/                  # Role A：词法分析器（待 A 实现）
│   ├── parser/                 # Role A：语法分析器（待 A 实现）
│   ├── semantic/               # Role B：语义分析器（已完成）
│   ├──gen/                # Role C：代码生成器（待 C 实现）
│   └── opt/                    # Role D：优化器（待 D 实现）
├── tests/
│ ├── semantic/               # Role B 的语义测试（已完成）
│   ├── codegen/                # Role C 代码生成测试
│   └──/            # 端到端集成测试
├── docs/                       # 设计文档
└── README.md                   # 本文件
```

### 2. 主入口串联时序

各角色在 main.cpp 中的调用（已在本文件"完整编译流水线"给出代码模板）：

```
 1: [A] 词法分析 → 语法分析 → 构建 AST
步骤 2: [B] SemanticAnalyzer::analyze(ast) → 如有错误则终止
步骤 3: [C] CodeGenerator::generate(ast, analyzer) → 生成代码
步骤 : [D] Optim::optimize() → 代码优化（可选）
```

**关键边界条件：**
- 步骤 2 未通过 → 不执行步骤 34
- 步骤 2 通过后，C 和 D 均可通过 analyzer.lookupGlobal/Local() 查询符号信息
- D 可在优化阶段复用 ConstExprEvaluator 实例进行常量折叠

### 3. 编译链接方式

合并后的 CMakeLists.txt（四角色）：

```ake
cmake_minimum_required(VERSION 3.20)
project(ToyCCompiler LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
 # Role A：前端
    src/lexer/Lexer.cpp          # A 提供
    src/parser/Parser.cpp        # A 提供
    # Role B：语义分析（已完成）
    src/semantic/Symbol.cpp
    src/sem/SymbolTable.cpp
    src/semantic/ConstExprEvaluator.cpp
    src/semantic/SemanticAnalyzer.cpp
    # Role C：后端
    src/codegen/CodeGenerator.cpp  # 提供
    # Role D：优化
    src/opt/Optimizer.cpp          # D 提供
)

add_executable(toyc ${SOURCES})
target_includeirectories(toyc PRIVATE src)
target_compile_options(toyc PRIVATE -Wall -Wextra -pedantic)
```

### 4. 角色协作检查清单

| 项 | 涉及角色 | 状态 |
|-------|---------|:----:|
| A 确认 AST 节点字段（line/column/type/isStatement） | A ↔ B | 待 A 补充字段 |
| B 的 SemanticAnalyzer 接口（analyze/lookupGlobal/lookup） | B → C | 已定义 |
| B 的 getConstantValue / isUsed 供优化复用 B → D | 已定义| C 填充 Symbol::offset 栈帧偏移 | C → B | C 需 |
| D 复用 ConstExprEvaluator 做表达式折叠 | D → B | D 需注意方式 |
