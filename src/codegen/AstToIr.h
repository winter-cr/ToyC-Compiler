#pragma once

#include "ast/AST.h"
#include "toyc/backend/ir.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace toyc {

class AstToIr : public ASTVisitor {
public:
    AstToIr();

    toyc::backend::Program convert(CompUnit* ast);

    // ASTVisitor overrides
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
    toyc::backend::Value emitExpr(Expr* expr, toyc::backend::FunctionBuilder& builder);
    toyc::backend::BinaryOp mapBinaryOp(toyc::BinaryOp op);
    toyc::backend::UnaryOp mapUnaryOp(toyc::UnaryOp op);
    std::optional<toyc::backend::Value> lookup(const std::string& name) const;

    toyc::backend::Program program_;
    toyc::backend::FunctionBuilder* builder_ = nullptr;

    // Per-function state
    std::unordered_map<std::string, toyc::backend::Value> locals_;
    std::unordered_map<std::string, bool> global_names_;  // O(1) lookup
    std::string current_func_name_;
};

} // namespace toyc
