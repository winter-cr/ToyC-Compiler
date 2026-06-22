#pragma once

#include "SymbolTable.h"
#include "Symbol.h"
#include <vector>
#include <string>

// 前向声明AST类型（由前端A定义）
namespace ast {
class ASTNode;
class CompUnit;
class FuncDef;
class VarDecl;
class ConstDecl;
class Block;
class Stmt;
class Expr;
}

namespace semantic {

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer();
    ~SemanticAnalyzer();

    // 主入口：分析AST，返回是否通过
    bool analyze(ast::ASTNode* root);

    // 错误收集
    const std::vector<SemanticError>& errors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

    // 符号查询（供后端C使用）
    Symbol* lookupGlobal(const std::string& name);
    Symbol* lookupLocal(const std::string& name);  // 当前函数作用域

    // 常量传播接口（供D优化使用）
    bool getConstantValue(const std::string& name, int* out_value);

    // 错误报告
    void addError(int line, int column, SemanticErrorCode code, const std::string& msg);

    // 重置分析状态
    void reset();

private:
    SymbolTable symbolTable_;     // 符号表
    std::vector<SemanticError> errors_;  // 错误列表

    // AST遍历辅助方法
    void visitCompUnit(ast::CompUnit* node);
    void visitFuncDef(ast::FuncDef* func);
    void visitVarDecl(ast::VarDecl* decl);
    void visitConstDecl(ast::ConstDecl* decl);
    void visitBlock(ast::Block* block);
    void visitStmt(ast::Stmt* stmt);
    void visitExpr(ast::Expr* expr);

    // 检查main函数签名
    void checkMainFunction();

    // 返回路径检查（简化版，后续扩展）
    bool checkReturnPaths(ast::Block* block, bool expectReturn);

    // 常量表达式求值（初步，第2天完善）
    std::optional<int> evaluateConstExpr(ast::Expr* expr);

    // 当前函数返回类型（用于返回路径检查）
    BasicType currentFuncReturnType_;
    bool inFunction_;            // 是否在函数体内
};

}  // namespace semantic
