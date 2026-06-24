#pragma once

#include "ast/AstVisitor.h"

#include <iostream>
#include <string>

class AstPrinter : public AstVisitor {
public:
    explicit AstPrinter(std::ostream& out) : out_(out) {}

    void print(const CompUnit& node);

    void visit(const CompUnit& node) override;
    void visit(const VarDecl& node) override;
    void visit(const ConstDecl& node) override;
    void visit(const FuncDef& node) override;
    void visit(const Param& node) override;
    void visit(const BlockStmt& node) override;
    void visit(const EmptyStmt& node) override;
    void visit(const ExprStmt& node) override;
    void visit(const AssignStmt& node) override;
    void visit(const DeclStmt& node) override;
    void visit(const IfStmt& node) override;
    void visit(const WhileStmt& node) override;
    void visit(const BreakStmt& node) override;
    void visit(const ContinueStmt& node) override;
    void visit(const ReturnStmt& node) override;
    void visit(const BinaryExpr& node) override;
    void visit(const UnaryExpr& node) override;
    void visit(const IntLiteral& node) override;
    void visit(const IdentifierExpr& node) override;
    void visit(const CallExpr& node) override;

private:
    void indent();
    void push();
    void pop();

    std::ostream& out_;
    int depth_ = 0;
};
