#pragma once

#include "ast/AstNode.h"
#include "ast/Decl.h"
#include "ast/Expr.h"

#include <memory>
#include <vector>

class AstVisitor;

class Stmt : public AstNode {
protected:
    explicit Stmt(AstNodeKind kind, SourceLocation loc) : AstNode(kind, loc) {}
};

class BlockStmt : public Stmt {
public:
    explicit BlockStmt(SourceLocation loc, std::vector<std::unique_ptr<Stmt>> stmts)
        : Stmt(AstNodeKind::BlockStmt, loc), stmts_(std::move(stmts)) {}

    const std::vector<std::unique_ptr<Stmt>>& stmts() const { return stmts_; }

    void accept(AstVisitor& visitor) const override;

private:
    std::vector<std::unique_ptr<Stmt>> stmts_;
};

class EmptyStmt : public Stmt {
public:
    explicit EmptyStmt(SourceLocation loc) : Stmt(AstNodeKind::EmptyStmt, loc) {}

    void accept(AstVisitor& visitor) const override;
};

class ExprStmt : public Stmt {
public:
    ExprStmt(SourceLocation loc, std::unique_ptr<Expr> expr)
        : Stmt(AstNodeKind::ExprStmt, loc), expr_(std::move(expr)) {}

    const Expr& expr() const { return *expr_; }

    void accept(AstVisitor& visitor) const override;

private:
    std::unique_ptr<Expr> expr_;
};

class AssignStmt : public Stmt {
public:
    AssignStmt(SourceLocation loc, std::string name, std::unique_ptr<Expr> value)
        : Stmt(AstNodeKind::AssignStmt, loc), name_(std::move(name)), value_(std::move(value)) {}

    const std::string& name() const { return name_; }
    const Expr& value() const { return *value_; }

    void accept(AstVisitor& visitor) const override;

private:
    std::string name_;
    std::unique_ptr<Expr> value_;
};

class DeclStmt : public Stmt {
public:
    DeclStmt(SourceLocation loc, std::unique_ptr<Decl> decl)
        : Stmt(AstNodeKind::DeclStmt, loc), decl_(std::move(decl)) {}

    const Decl& decl() const { return *decl_; }

    void accept(AstVisitor& visitor) const override;

private:
    std::unique_ptr<Decl> decl_;
};

class IfStmt : public Stmt {
public:
    IfStmt(SourceLocation loc,
           std::unique_ptr<Expr> condition,
           std::unique_ptr<Stmt> then_branch,
           std::unique_ptr<Stmt> else_branch)
        : Stmt(AstNodeKind::IfStmt, loc),
          condition_(std::move(condition)),
          then_branch_(std::move(then_branch)),
          else_branch_(std::move(else_branch)) {}

    const Expr& condition() const { return *condition_; }
    const Stmt& then_branch() const { return *then_branch_; }
    bool has_else() const { return else_branch_ != nullptr; }
    const Stmt* else_branch() const { return else_branch_.get(); }

    void accept(AstVisitor& visitor) const override;

private:
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<Stmt> then_branch_;
    std::unique_ptr<Stmt> else_branch_;
};

class WhileStmt : public Stmt {
public:
    WhileStmt(SourceLocation loc, std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
        : Stmt(AstNodeKind::WhileStmt, loc),
          condition_(std::move(condition)),
          body_(std::move(body)) {}

    const Expr& condition() const { return *condition_; }
    const Stmt& body() const { return *body_; }

    void accept(AstVisitor& visitor) const override;

private:
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<Stmt> body_;
};

class BreakStmt : public Stmt {
public:
    explicit BreakStmt(SourceLocation loc) : Stmt(AstNodeKind::BreakStmt, loc) {}

    void accept(AstVisitor& visitor) const override;
};

class ContinueStmt : public Stmt {
public:
    explicit ContinueStmt(SourceLocation loc) : Stmt(AstNodeKind::ContinueStmt, loc) {}

    void accept(AstVisitor& visitor) const override;
};

class ReturnStmt : public Stmt {
public:
    ReturnStmt(SourceLocation loc, std::unique_ptr<Expr> value)
        : Stmt(AstNodeKind::ReturnStmt, loc), value_(std::move(value)) {}

    bool has_value() const { return value_ != nullptr; }
    const Expr* value() const { return value_.get(); }

    void accept(AstVisitor& visitor) const override;

private:
    std::unique_ptr<Expr> value_;
};
