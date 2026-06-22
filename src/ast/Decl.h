#pragma once

#include "ast/AstNode.h"
#include "ast/Expr.h"

#include <memory>
#include <string>

class AstVisitor;

class Decl : public AstNode {
protected:
    explicit Decl(AstNodeKind kind, SourceLocation loc) : AstNode(kind, loc) {}
};

class VarDecl : public Decl {
public:
    VarDecl(SourceLocation loc, std::string name, std::unique_ptr<Expr> init)
        : Decl(AstNodeKind::VarDecl, loc), name_(std::move(name)), init_(std::move(init)) {}

    const std::string& name() const { return name_; }
    const Expr& init() const { return *init_; }

    void accept(AstVisitor& visitor) const override;

private:
    std::string name_;
    std::unique_ptr<Expr> init_;
};

class ConstDecl : public Decl {
public:
    ConstDecl(SourceLocation loc, std::string name, std::unique_ptr<Expr> init)
        : Decl(AstNodeKind::ConstDecl, loc), name_(std::move(name)), init_(std::move(init)) {}

    const std::string& name() const { return name_; }
    const Expr& init() const { return *init_; }

    void accept(AstVisitor& visitor) const override;

private:
    std::string name_;
    std::unique_ptr<Expr> init_;
};
