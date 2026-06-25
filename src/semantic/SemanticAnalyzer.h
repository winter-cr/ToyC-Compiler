#ifndef TOYC_SEMANTIC_ANALYZER_H
#define TOYC_SEMANTIC_ANALYZER_H

#include "ast/AST.h"
#include "semantic/errors.h"
#include "semantic/Symbol.h"
#include "semantic/SymbolTable.h"
#include <optional>
#include <string>
#include <vector>

namespace toyc {

class SemanticAnalyzer : public ASTVisitor {
public:
    SemanticAnalyzer();

    bool analyze(CompUnit* root);
    bool hasErrors() const;
    const std::vector<SemanticError>& errors() const;

    Symbol* lookupGlobal(const std::string& name) const;
    Symbol* lookupLocal(const std::string& name) const;
    bool getConstantValue(const std::string& name, int* outValue) const;

    void visit(CompUnit* node) override;
    void visit(VarDecl* node) override;
    void visit(ConstDecl* node) override;
    void visit(FuncDef* node) override;
    void visit(Param* node) override;
    void visit(Block* node) override;
    void visit(IfStmt* node) override;
    void visit(WhileStmt* node) override;
    void visit(BreakStmt* node) override;
    void visit(ContinueStmt* node) override;
    void visit(ReturnStmt* node) override;
    void visit(ExprStmt* node) override;
    void visit(AssignStmt* node) override;
    void visit(EmptyStmt* node) override;
    void visit(BinaryExpr* node) override;
    void visit(UnaryExpr* node) override;
    void visit(Identifier* node) override;
    void visit(Number* node) override;
    void visit(FuncCall* node) override;

private:
    SymbolTable symTable_;
    std::vector<SemanticError> errors_;
    FuncDef* currentFunction_ = nullptr;
    int loopDepth_ = 0;
    bool hasMain_ = false;
    bool inConstantContext_ = false;

    void error(int line, int col, SemanticErrorCode code,
               const std::string& msg);
    Symbol* declareVar(const std::string& name, SymbolKind kind,
                       int line, bool isGlobal,
                       std::optional<int> constVal = std::nullopt);
    Symbol* lookupSymbol(const std::string& name);
    void checkExpr(Expr* expr);
    std::optional<int> evaluateConstExpr(Expr* expr);
    bool checkReturnPaths(Block* block, bool expectReturn);
    std::string errorMessage(SemanticErrorCode code);
};

} // namespace toyc

#endif
