# 语义分析模块（B任务）- 第一天交付物

## 模块结构

```
src/semantic/
  ├── Symbol.h/cpp          # Symbol类定义（种类、类型、常量值）
  ├── SymbolTable.h/cpp     # 作用域管理（栈+哈希表）
  ├── SemanticAnalyzer.h/cpp  # 主分析器（骨架）
  └── errors.h             # 错误码定义

tests/semantic/
  ├── valid/               # 合法样例（5个）
  │   ├── 01_empty_main.tc
  │   ├── 02_simple_const.tc
  │   ├── 03_global_var.tc
  │   ├── 04_local_var.tc
  │   └── 05_shadowing.tc
  └── invalid/             # 非法样例（5个）
      ├── 01_undeclared_var.tc
      ├── 02_duplicate_global.tc
      ├── 03_const_reassign.tc
      ├── 04_duplicate_local.tc
      └── 05_use_before_decl.tc
```

## 编译

本项目使用CMake。首先确保根目录有 CMakeLists.txt，然后：

```bash
mkdir build
cd build
cmake ..
make
```

可执行文件 `compiler` 会生成在 build/ 目录下。

## 运行测试

语义分析模块会读取 ToyC 源码（.tc文件），进行语义检查，错误输出到stderr。

```bash
# 测试合法程序 - 应无错误输出，退出码0
./compiler < tests/semantic/valid/01_empty_main.tc > /dev/null
echo $?  # 输出 0

# 测试非法程序 - 应有错误输出，退出码非0
./compiler < tests/semantic/invalid/01_undeclared_var.tc 2> err.txt
cat err.txt
echo $?  # 输出非0
```

## 当前实现状态

### ✅ 已完成
- Symbol 类定义（kind, type, isConst, constValue, offset, isGlobal, isUsed）
- SymbolTable 作用域管理（enterScope, exitScope, insert, lookup）
- SemanticAnalyzer 骨架（analyze, 错误管理, 符号查询接口）
- 错误码枚举（15种语义错误类型）

### ⏳ 待实现（第2-6天）
- CompUnit遍历（收集全局符号）
- ConstExprEvaluator（常量表达式求值）
- 函数局部作用域处理
- 返回路径检查、break/continue检查
- void函数调用限制
- 完整错误诊断

## 接口说明

### SemanticAnalyzer 主类

```cpp
// 分析入口（AST由前端A提供）
bool analyze(ast::ASTNode* root);

// 符号查询（供后端C使用）
Symbol* lookupGlobal(const std::string& name);
Symbol* lookupLocal(const std::string& name);

// 常量传播（供优化D使用）
bool getConstantValue(const std::string& name, int* out_value);

// 错误信息
const std::vector<SemanticError>& errors() const;
```

### Symbol 结构

```cpp
struct Symbol {
    SymbolKind kind;             // GLOBAL_VAR, GLOBAL_CONST, LOCAL_VAR, PARAM, FUNCTION...
    std::string name;
    BasicType type;              // INT or VOID
    bool isConst;                // 是否为const
    std::optional<int> constValue;  // 常量值（如果已知）
    int line;                    // 声明行号
    int offset;                  // 栈帧偏移（由后端C填充）
    bool isGlobal;               // 是否全局
    bool isUsed;                 // 死变量检测（由D使用）
};
```

## 与前端（A）的接口约定

AST节点必须提供：
- `line`, `column` 源码位置
- 表达式节点 `Expr::type` 类型信息
- 声明节点 `Decl::initExpr` 初始化表达式
- 赋值语句标记左值（用于const检查）
- 函数调用节点标记是否语句上下文（用于void函数限制）

## 错误信息格式

```
{file}:{line}:{column}: error: [SEM_XXX] {description}
```

示例：
```
test.tc:3:5: error: [SEM_UNDECLARED_IDENT] use of undeclared identifier 'x'
```

## 下一步任务

- 第2天：实现全局符号收集和常量表达式求值
- 第3天：实现函数局部作用域和符号插入
- 第4天：实现break/continue、返回路径、void函数调用检查
- 第5天：提供常量传播接口，复查所有规则
- 第6天：编写设计文档和实践报告

---

**负责人**：B（语义分析）  
**最后更新**：第1天完成
