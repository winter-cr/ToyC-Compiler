#pragma once

#include "ast/ASTBase.h"
#include "ast/Type.h"

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace toyc {

class Block;

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

class CompUnit;
class VarDecl;
class ConstDecl;
class FuncDef;
class Param;
class ExprStmt;
class AssignStmt;
class IfStmt;
class WhileStmt;
class BreakStmt;
class ContinueStmt;
class ReturnStmt;
class BinaryExpr;
class UnaryExpr;
class Number;
class Identifier;
class FuncCall;

class BinaryExpr : public Expr {
public:
    std::unique_ptr<Expr> left;
    BinaryOp op;
    std::unique_ptr<Expr> right;

    BinaryExpr(int line, int column, BinaryOp op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
        : Expr(line, column, nullptr),
          left(std::move(left)),
          op(op),
          right(std::move(right)) {}

    void accept(ASTVisitor& visitor) const override;
};

class UnaryExpr : public Expr {
public:
    UnaryOp op;
    std::unique_ptr<Expr> operand;

    UnaryExpr(int line, int column, UnaryOp op, std::unique_ptr<Expr> operand)
        : Expr(line, column, nullptr), op(op), operand(std::move(operand)) {}

    void accept(ASTVisitor& visitor) const override;
};

class Number : public Expr {
public:
    int value;

    Number(int line, int column, int value)
        : Expr(line, column, Type::intType()), value(value) {}

    void accept(ASTVisitor& visitor) const override;
};

class Identifier : public Expr {
public:
    std::string name;

    Identifier(int line, int column, std::string name)
        : Expr(line, column, nullptr), name(std::move(name)) {}

    void accept(ASTVisitor& visitor) const override;
};

using ID = Identifier;

class FuncCall : public Expr {
public:
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;
    bool isStatement;

    FuncCall(int line,
             int column,
             std::string name,
             std::vector<std::unique_ptr<Expr>> args,
             bool isStatement = false)
        : Expr(line, column, nullptr),
          name(std::move(name)),
          args(std::move(args)),
          isStatement(isStatement) {}

    void accept(ASTVisitor& visitor) const override;
};

class VarDecl : public Stmt {
public:
    std::string name;
    std::unique_ptr<Expr> initExpr;
    bool isGlobal;

    VarDecl(int line, int column, std::string name, std::unique_ptr<Expr> initExpr, bool isGlobal)
        : Stmt(line, column),
          name(std::move(name)),
          initExpr(std::move(initExpr)),
          isGlobal(isGlobal) {}

    void accept(ASTVisitor& visitor) const override;
};

class ConstDecl : public Stmt {
public:
    std::string name;
    std::unique_ptr<Expr> initExpr;
    bool isGlobal;

    ConstDecl(int line, int column, std::string name, std::unique_ptr<Expr> initExpr, bool isGlobal)
        : Stmt(line, column),
          name(std::move(name)),
          initExpr(std::move(initExpr)),
          isGlobal(isGlobal) {}

    void accept(ASTVisitor& visitor) const override;
};

class Param : public ASTNode {
public:
    std::string name;

    Param(int line, int column, std::string name)
        : ASTNode(line, column), name(std::move(name)) {}

    void accept(ASTVisitor& visitor) const override;
};

class Block : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit Block(int line, int column) : Stmt(line, column) {}

    void accept(ASTVisitor& visitor) const override;
};

class FuncDef : public ASTNode {
public:
    Type* returnType;
    std::string name;
    std::vector<std::unique_ptr<Param>> params;
    std::unique_ptr<Block> body;

    FuncDef(int line,
            int column,
            Type* returnType,
            std::string name,
            std::vector<std::unique_ptr<Param>> params,
            std::unique_ptr<Block> body)
        : ASTNode(line, column),
          returnType(returnType),
          name(std::move(name)),
          params(std::move(params)),
          body(std::move(body)) {}

    void accept(ASTVisitor& visitor) const override;
};

class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expr;

    ExprStmt(int line, int column, std::unique_ptr<Expr> expr)
        : Stmt(line, column), expr(std::move(expr)) {}

    void accept(ASTVisitor& visitor) const override;
};

class AssignStmt : public Stmt {
public:
    std::unique_ptr<Identifier> lvalue;
    std::unique_ptr<Expr> value;

    AssignStmt(int line, int column, std::unique_ptr<Identifier> lvalue, std::unique_ptr<Expr> value)
        : Stmt(line, column), lvalue(std::move(lvalue)), value(std::move(value)) {}

    void accept(ASTVisitor& visitor) const override;
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> then;
    std::unique_ptr<Stmt> else_;

    IfStmt(int line,
           int column,
           std::unique_ptr<Expr> cond,
           std::unique_ptr<Stmt> then,
           std::unique_ptr<Stmt> else_)
        : Stmt(line, column),
          cond(std::move(cond)),
          then(std::move(then)),
          else_(std::move(else_)) {}

    bool has_else() const { return else_ != nullptr; }

    void accept(ASTVisitor& visitor) const override;
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> body;

    WhileStmt(int line, int column, std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> body)
        : Stmt(line, column), cond(std::move(cond)), body(std::move(body)) {}

    void accept(ASTVisitor& visitor) const override;
};

class BreakStmt : public Stmt {
public:
    explicit BreakStmt(int line, int column) : Stmt(line, column) {}

    void accept(ASTVisitor& visitor) const override;
};

class ContinueStmt : public Stmt {
public:
    explicit ContinueStmt(int line, int column) : Stmt(line, column) {}

    void accept(ASTVisitor& visitor) const override;
};

class ReturnStmt : public Stmt {
public:
    std::unique_ptr<Expr> value;

    ReturnStmt(int line, int column, std::unique_ptr<Expr> value)
        : Stmt(line, column), value(std::move(value)) {}

    bool has_value() const { return value != nullptr; }

    void accept(ASTVisitor& visitor) const override;
};

using TopLevelElement = std::variant<std::unique_ptr<VarDecl>,
                                     std::unique_ptr<ConstDecl>,
                                     std::unique_ptr<FuncDef>>;

class CompUnit : public ASTNode {
public:
    std::vector<TopLevelElement> elements;

    CompUnit() : ASTNode(1, 1) {}
    CompUnit(int line, int column) : ASTNode(line, column) {}

    void accept(ASTVisitor& visitor) const override;
};

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(const CompUnit& node) = 0;
    virtual void visit(const VarDecl& node) = 0;
    virtual void visit(const ConstDecl& node) = 0;
    virtual void visit(const FuncDef& node) = 0;
    virtual void visit(const Param& node) = 0;
    virtual void visit(const Block& node) = 0;
    virtual void visit(const ExprStmt& node) = 0;
    virtual void visit(const AssignStmt& node) = 0;
    virtual void visit(const IfStmt& node) = 0;
    virtual void visit(const WhileStmt& node) = 0;
    virtual void visit(const BreakStmt& node) = 0;
    virtual void visit(const ContinueStmt& node) = 0;
    virtual void visit(const ReturnStmt& node) = 0;
    virtual void visit(const BinaryExpr& node) = 0;
    virtual void visit(const UnaryExpr& node) = 0;
    virtual void visit(const Number& node) = 0;
    virtual void visit(const Identifier& node) = 0;
    virtual void visit(const FuncCall& node) = 0;
};

const char* binary_op_name(BinaryOp op);
const char* unary_op_name(UnaryOp op);

} // namespace toyc
