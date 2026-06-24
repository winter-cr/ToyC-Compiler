#pragma once

#include "SymbolTable.h"
#include "Symbol.h"
#include "ConstExprEvaluator.h"
#include "AST.h"
#include <optional>
#include <vector>

namespace semantic {

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer();
    ~SemanticAnalyzer();

    bool analyze(ast::ASTNode* root);
    const std::vector<SemanticError>& errors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

    Symbol* lookupGlobal(const std::string& name);
    Symbol* lookupLocal(const std::string& name);
    bool getConstantValue(const std::string& name, int* out_value);

    void reset();
    void addError(int line, int column, SemanticErrorCode code, const std::string& msg);

private:
    SymbolTable symbolTable_;
    std::vector<SemanticError> errors_;
    ConstExprEvaluator constExprEval_;
    int loopDepth_ = 0;
    Symbol* currentFunction_ = nullptr;
    bool hasReturnInPath_ = false;

    void visitCompUnit(ast::CompUnit* node);
    void visitFuncDef(ast::FuncDef* func);
    void visitVarDecl(ast::VarDecl* decl, bool isGlobal);
    void visitConstDecl(ast::ConstDecl* decl, bool isGlobal);
    void checkMainFunction();
    void visitBlock(ast::Block* block, bool expectReturn = false);
    void visitStmt(ast::Stmt* stmt, bool expectReturn = false);
    Symbol* visitExpr(ast::Expr* expr);
    Symbol* visitID(ast::Identifier* id);
    bool hasReturnOnAllPaths(ast::Block* block);
};

} // namespace semantic
