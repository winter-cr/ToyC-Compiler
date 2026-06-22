#include "SemanticAnalyzer.h"
#include "errors.h"
#include <iostream>

namespace semantic {

SemanticAnalyzer::SemanticAnalyzer()
    : currentFuncReturnType_(BasicType::INT), inFunction_(false) {
}

SemanticAnalyzer::~SemanticAnalyzer() {
    reset();
}

void SemanticAnalyzer::reset() {
    symbolTable_.clear();
    errors_.clear();
    currentFuncReturnType_ = BasicType::INT;
    inFunction_ = false;
}

bool SemanticAnalyzer::analyze(ast::ASTNode* root) {
    reset();
    if (root) {
        visitCompUnit(static_cast<ast::CompUnit*>(root));
    }
    return hasErrors();
}

// ==================== 错误报告 ====================

void SemanticAnalyzer::addError(int line, int column, SemanticErrorCode code, const std::string& msg) {
    SemanticError err(line, column, code, msg);
    errors_.push_back(err);
}

// ==================== 符号查询接口 ====================

Symbol* SemanticAnalyzer::lookupGlobal(const std::string& name) {
    // 查找全局符号（任何作用域都可用，但只从全局作用域开始查）
    // 实际lookup会搜索所有作用域，这里直接返回lookup结果即可
    return symbolTable_.lookup(name);
}

Symbol* SemanticAnalyzer::lookupLocal(const std::string& name) {
    // 查找当前函数作用域的局部符号（包括形参、局部变量）
    // 从当前作用域向上查找，直到函数作用域
    return symbolTable_.lookup(name);
}

bool SemanticAnalyzer::getConstantValue(const std::string& name, int* out_value) {
    Symbol* sym = lookupGlobal(name);
    if (sym && sym->isConstant() && sym->getConstValue().has_value()) {
        *out_value = sym->getConstValue().value();
        return true;
    }
    return false;
}

// ==================== AST遍历（骨架实现）====================

void SemanticAnalyzer::visitCompUnit(ast::CompUnit* node) {
    // TODO: 实现全局声明和函数定义遍历
    // 第2天：
    //   - 收集全局变量、常量、函数声明
    //   - 检查重定义
    //   - 检查main函数签名
}

void SemanticAnalyzer::visitFuncDef(ast::FuncDef* func) {
    // TODO: 第3天实现
    //   - 创建新作用域
    //   - 插入形参
    //   - 遍历函数体
    //   - 检查返回路径
}

void SemanticAnalyzer::visitVarDecl(ast::VarDecl* decl) {
    // TODO: 第3天实现
    //   - 检查是否已声明（当前作用域）
    //   - 插入符号
    //   - 检查const不能左值（需Assignment标记）
}

void SemanticAnalyzer::visitConstDecl(ast::ConstDecl* decl) {
    // TODO: 第2-3天实现
    //   - 检查是否已声明
    //   - 常量表达式求值
    //   - 插入符号并填充constValue
}

void SemanticAnalyzer::visitBlock(ast::Block* block) {
    // TODO: 第3-4天实现
    //   - 进入新作用域（块作用域）
    //   - 遍历所有语句
    //   - 退出作用域
}

void SemanticAnalyzer::visitStmt(ast::Stmt* stmt) {
    // TODO: 按具体语句类型分发
    //   - if/while: 检查条件表达式
    //   - break/continue: 检查是否在循环内
    //   - return: 检查类型匹配
    //   - assignment: 检查左值是否const
}

void SemanticAnalyzer::visitExpr(ast::Expr* expr) {
    // TODO: 表达式类型检查
    //   - 函数调用：参数数量匹配、void函数调用限制
    //   - 变量引用：检查已声明
    //   - 二元运算：类型检查（目前都是int）
}

void SemanticAnalyzer::checkMainFunction() {
    // TODO: 第2天实现
    // 查找main函数，检查签名: int main(void)
    Symbol* mainSym = symbolTable_.lookup("main");
    if (!mainSym) {
        addError(1, 1, SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE,
                 "missing 'main' function");
        return;
    }
    // 检查返回类型是否为int，参数列表是否为空
    // ...
}

bool SemanticAnalyzer::checkReturnPaths(ast::Block* block, bool expectReturn) {
    // TODO: 第4天实现完整数据流分析
    // 简化版：目前返回true（假设都有返回）
    return true;
}

std::optional<int> SemanticAnalyzer::evaluateConstExpr(ast::Expr* expr) {
    // TODO: 第2天实现常量表达式求值
    // 支持：NUMBER, ID(已声明的常量), + - * / %, && || !, < > <= >= == !=
    // 短路逻辑：&& 左假返回0，|| 左真返回1
    // 除零检查
    return std::nullopt;
}

}  // namespace semantic
