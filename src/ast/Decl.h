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
    VarDecl(SourceLocation loc, std::string name, std::unique_ptr<Expr> init, bool is_global)
        : Decl(AstNodeKind::VarDecl, loc),
          name_(std::move(name)),
          init_(std::move(init)),
          is_global_(is_global) {}

    const std::string& name() const { return name_; }
    const Expr& init() const { return *init_; }
    const Expr& init_expr() const { return *init_; }
    bool is_global() const { return is_global_; }

    void accept(AstVisitor& visitor) const override;

private:
    std::string name_;
    std::unique_ptr<Expr> init_;
    bool is_global_;
};

class ConstDecl : public Decl {
public:
    ConstDecl(SourceLocation loc, std::string name, std::unique_ptr<Expr> init, bool is_global)
        : Decl(AstNodeKind::ConstDecl, loc),
          name_(std::move(name)),
          init_(std::move(init)),
          is_global_(is_global) {}

    const std::string& name() const { return name_; }
    const Expr& init() const { return *init_; }
    const Expr& init_expr() const { return *init_; }
    bool is_global() const { return is_global_; }

    void accept(AstVisitor& visitor) const override;

private:
    std::string name_;
    std::unique_ptr<Expr> init_;
    bool is_global_;
};
