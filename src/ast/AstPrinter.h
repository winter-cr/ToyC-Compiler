#pragma once

#include "ast/AST.h"

#include <iostream>
#include <string>

class AstPrinter : public toyc::ASTVisitor {
public:
    explicit AstPrinter(std::ostream& out) : out_(out) {}

    void print(const toyc::CompUnit& node);

    void visit(const toyc::CompUnit& node) override;
    void visit(const toyc::VarDecl& node) override;
    void visit(const toyc::ConstDecl& node) override;
    void visit(const toyc::FuncDef& node) override;
    void visit(const toyc::Param& node) override;
    void visit(const toyc::Block& node) override;
    void visit(const toyc::ExprStmt& node) override;
    void visit(const toyc::AssignStmt& node) override;
    void visit(const toyc::IfStmt& node) override;
    void visit(const toyc::WhileStmt& node) override;
    void visit(const toyc::BreakStmt& node) override;
    void visit(const toyc::ContinueStmt& node) override;
    void visit(const toyc::ReturnStmt& node) override;
    void visit(const toyc::BinaryExpr& node) override;
    void visit(const toyc::UnaryExpr& node) override;
    void visit(const toyc::Number& node) override;
    void visit(const toyc::Identifier& node) override;
    void visit(const toyc::FuncCall& node) override;

private:
    void indent();
    void push();
    void pop();

    std::ostream& out_;
    int depth_ = 0;
};
