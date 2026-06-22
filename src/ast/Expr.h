#pragma once

#include "ast/AstNode.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class AstVisitor;

enum class BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Lt,
    Gt,
    Le,
    Ge,
    Eq,
    Ne,
    And,
    Or,
};

enum class UnaryOp {
    Plus,
    Minus,
    Not,
};

class Expr : public AstNode {
protected:
    explicit Expr(AstNodeKind kind, SourceLocation loc) : AstNode(kind, loc) {}
};

class BinaryExpr : public Expr {
public:
    BinaryExpr(SourceLocation loc, BinaryOp op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
        : Expr(AstNodeKind::BinaryExpr, loc), op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    BinaryOp op() const { return op_; }
    const Expr& lhs() const { return *lhs_; }
    const Expr& rhs() const { return *rhs_; }

    void accept(AstVisitor& visitor) const override;

private:
    BinaryOp op_;
    std::unique_ptr<Expr> lhs_;
    std::unique_ptr<Expr> rhs_;
};

class UnaryExpr : public Expr {
public:
    UnaryExpr(SourceLocation loc, UnaryOp op, std::unique_ptr<Expr> operand)
        : Expr(AstNodeKind::UnaryExpr, loc), op_(op), operand_(std::move(operand)) {}

    UnaryOp op() const { return op_; }
    const Expr& operand() const { return *operand_; }

    void accept(AstVisitor& visitor) const override;

private:
    UnaryOp op_;
    std::unique_ptr<Expr> operand_;
};

class IntLiteral : public Expr {
public:
    IntLiteral(SourceLocation loc, int64_t value)
        : Expr(AstNodeKind::IntLiteral, loc), value_(value) {}

    int64_t value() const { return value_; }

    void accept(AstVisitor& visitor) const override;

private:
    int64_t value_;
};

class IdentifierExpr : public Expr {
public:
    IdentifierExpr(SourceLocation loc, std::string name)
        : Expr(AstNodeKind::IdentifierExpr, loc), name_(std::move(name)) {}

    const std::string& name() const { return name_; }

    void accept(AstVisitor& visitor) const override;

private:
    std::string name_;
};

class CallExpr : public Expr {
public:
    CallExpr(SourceLocation loc, std::string name, std::vector<std::unique_ptr<Expr>> args)
        : Expr(AstNodeKind::CallExpr, loc), name_(std::move(name)), args_(std::move(args)) {}

    const std::string& name() const { return name_; }
    const std::vector<std::unique_ptr<Expr>>& args() const { return args_; }

    void accept(AstVisitor& visitor) const override;

private:
    std::string name_;
    std::vector<std::unique_ptr<Expr>> args_;
};

const char* binary_op_name(BinaryOp op);
const char* unary_op_name(UnaryOp op);
