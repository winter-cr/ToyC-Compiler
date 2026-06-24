#pragma once

#include <string>

struct SourceLocation {
    int line = 1;
    int column = 1;
};

enum class AstNodeKind {
    CompUnit,

    VarDecl,
    ConstDecl,

    FuncDef,
    Param,

    BlockStmt,
    EmptyStmt,
    ExprStmt,
    AssignStmt,
    DeclStmt,
    IfStmt,
    WhileStmt,
    BreakStmt,
    ContinueStmt,
    ReturnStmt,

    BinaryExpr,
    UnaryExpr,
    IntLiteral,
    IdentifierExpr,
    CallExpr,
};

class AstVisitor;

class AstNode {
public:
    explicit AstNode(AstNodeKind kind, SourceLocation loc) : kind_(kind), loc_(loc) {}
    virtual ~AstNode() = default;

    AstNodeKind kind() const { return kind_; }
    const SourceLocation& location() const { return loc_; }
    int line() const { return loc_.line; }
    int column() const { return loc_.column; }

    virtual void accept(AstVisitor& visitor) const = 0;

protected:
    AstNodeKind kind_;
    SourceLocation loc_;
};
