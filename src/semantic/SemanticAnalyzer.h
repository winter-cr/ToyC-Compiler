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

    void visit(const CompUnit& node) override;
    void visit(const VarDecl& node) override;
    void visit(const ConstDecl& node) override;
    void visit(const FuncDef& node) override;
    void visit(const Param& node) override;
    void visit(const Block& node) override;
    void visit(const IfStmt& node) override;
    void visit(const WhileStmt& node) override;
    void visit(const BreakStmt& node) override;
    void visit(const ContinueStmt& node) override;
    void visit(const ReturnStmt& node) override;
    void visit(const ExprStmt& node) override;
    void visit(const AssignStmt& node) override;
    void visit(const BinaryExpr& node) override;
    void visit(const UnaryExpr& node) override;
    void visit(const Identifier& node) override;
    void visit(const Number& node) override;
    void visit(const FuncCall& node) override;

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
    void checkExpr(const Expr* expr);
    std::optional<int> evaluateConstExpr(Expr* expr);
    bool checkReturnPaths(const Block* block, bool expectReturn);
    std::string errorMessage(SemanticErrorCode code);
};

} // namespace toyc

#endif
