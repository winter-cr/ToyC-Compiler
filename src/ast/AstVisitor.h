#pragma once

#include "ast/CompUnit.h"
#include "ast/Decl.h"
#include "ast/Expr.h"
#include "ast/FuncDef.h"
#include "ast/Stmt.h"

class AstVisitor {
public:
    virtual ~AstVisitor() = default;

    virtual void visit(const CompUnit& node) = 0;
    virtual void visit(const VarDecl& node) = 0;
    virtual void visit(const ConstDecl& node) = 0;
    virtual void visit(const FuncDef& node) = 0;
    virtual void visit(const Param& node) = 0;
    virtual void visit(const BlockStmt& node) = 0;
    virtual void visit(const EmptyStmt& node) = 0;
    virtual void visit(const ExprStmt& node) = 0;
    virtual void visit(const AssignStmt& node) = 0;
    virtual void visit(const DeclStmt& node) = 0;
    virtual void visit(const IfStmt& node) = 0;
    virtual void visit(const WhileStmt& node) = 0;
    virtual void visit(const BreakStmt& node) = 0;
    virtual void visit(const ContinueStmt& node) = 0;
    virtual void visit(const ReturnStmt& node) = 0;
    virtual void visit(const BinaryExpr& node) = 0;
    virtual void visit(const UnaryExpr& node) = 0;
    virtual void visit(const IntLiteral& node) = 0;
    virtual void visit(const IdentifierExpr& node) = 0;
    virtual void visit(const CallExpr& node) = 0;
};
