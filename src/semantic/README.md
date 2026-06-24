# 语义分析模块（B任务）

## 模块结构

```
src/semantic/
  ├── Symbol.h/cpp          # Symbol类定义（种类、类型、常量值）
  ├── SymbolTable.h/cpp     # 作用域管理（栈+哈希表）
  ├── SemanticAnalyzer.h/cpp  # 主分析器（完整实现）
  ├── ConstExprEvaluator.h/cpp # 常量表达式求
  └── errors.h             # 错误码定义
```

## 当前实现状态

### Day 1-3 已完成
- Symbol 类定义（GLOBAL_VAR, GLOBAL_CONST, LOCAL_VAR, LOCAL_CONST, PARAM, FUNCTION）
- SymbolTable 作用域管理（enterScope, exitScope, insert, lookup, isDefinedInCurrentScope）
- SemanticAnalyzer 完整实现（CompUnit遍历、函数定义、语句与表达式检查）
- 错误码枚举（15种语义错误类型）
- 全局符号收集和常量表达式求值
- main函数签名检查
- **函数局部作用域处理（形参插入、作用域管理）**
- **语句级语义检查（return、赋值、if、while、break、continue、声明）**
- **表达式语义检查（标识符解析、函数调用校验、参数匹配）**
- **const赋值检查、void函数调用限制**

### 4-6 待实现
- 返回路径检查
- 完整错误诊断强化
- 常量接口（供优化模块使用）
-编写设计文档和实践报告

## 接口说明

### SemanticAnalyzer 主类
```
bool analyze(ast::ASTNode* root);
Symbol* lookupGlobal(const std::string& name);
Symbol* lookupLocal(const std::string& name);
bool getConstantValue(const std::string name, int* out_value);
const std::vector<SemanticError>& errors() const;
```

 错误信息格式
```
{file}:{line}:{column}: error: [SEM_XXX] {description}
```

---

**最后更新**：第3天完成
