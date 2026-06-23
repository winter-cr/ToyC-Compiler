#pragma once

#include "ast/AstNode.h"
#include "ast/Stmt.h"
#include "ast/Type.h"

#include <memory>
#include <string>
#include <vector>

class Param : public AstNode {
public:
    Param(SourceLocation loc, std::string name)
        : AstNode(AstNodeKind::Param, loc), name_(std::move(name)) {}

    const std::string& name() const { return name_; }

    void accept(AstVisitor& visitor) const override;

private:
    std::string name_;
};

class FuncDef : public AstNode {
public:
    FuncDef(SourceLocation loc,
            Type* return_type,
            std::string name,
            std::vector<std::unique_ptr<Param>> params,
            std::unique_ptr<BlockStmt> body)
        : AstNode(AstNodeKind::FuncDef, loc),
          return_type_(return_type),
          name_(std::move(name)),
          params_(std::move(params)),
          body_(std::move(body)) {}

    Type* return_type() const { return return_type_; }
    const std::string& name() const { return name_; }
    const std::vector<std::unique_ptr<Param>>& params() const { return params_; }
    const BlockStmt& body() const { return *body_; }

    void accept(AstVisitor& visitor) const override;

private:
    Type* return_type_;
    std::string name_;
    std::vector<std::unique_ptr<Param>> params_;
    std::unique_ptr<BlockStmt> body_;
};
