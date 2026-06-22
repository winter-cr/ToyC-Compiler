# 前端模块接口说明（成员 A）

## 模块职责

- `src/lexer/`：将 ToyC 源码扫描为 Token 序列
- `src/parser/`：递归下降语法分析，构建 AST
- `src/ast/`：AST 节点定义、访问者接口与调试打印

## 对外接口

```cpp
Lexer lexer(source_string);
const std::vector<Token>& tokens = lexer.tokens();

Parser parser(tokens);
std::unique_ptr<CompUnit> ast = parser.parse();
```

解析失败时抛出 `ParseError`，词法失败时抛出 `LexError`（输出前缀为 `lexical error:`）。

## AST 结构

- 顶层：`CompUnit`，元素为 `VarDecl` / `ConstDecl` / `FuncDef`
- 语句：`BlockStmt`、`IfStmt`、`WhileStmt`、`ReturnStmt` 等
- 表达式：按优先级分层，`BinaryExpr` 保留 `&&` / `||` 节点以支持短路语义

## 调试

设置环境变量 `TOYC_DUMP_AST=1` 可将 AST 打印到 stderr：

```powershell
$env:TOYC_DUMP_AST=1
Get-Content tests\basic\return_7.tc | .\build\toyc.exe
```

## 后续集成点

语义分析（成员 B）可通过 `AstVisitor` 遍历 AST；代码生成（成员 C）在 `main.cpp` 中于 `parser.parse()` 之后接入。
