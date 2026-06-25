#ifndef TOYC_AST_H
#define TOYC_AST_H

#include "ast/Type.h"
#include "ast/ASTBase.h"
#include <memory>
#include <string>
#include <vector>

namespace toyc {

// Expressions
class BinaryExpr : public Expr {
public:
    enum class Op {
        ADD, SUB, MUL, DIV, MOD,
        EQ, NE, LT, LE, GT, GE,
        AND, OR
    };
    std::unique_ptr<Expr> left;
    Op op;
    std::unique_ptr<Expr> right;
    BinaryExpr(std::unique_ptr<Expr> l, Op o, std::unique_ptr<Expr> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
    void accept(ASTVisitor& v) override;
    static Op opFromString(const std::string& s);
};

class UnaryExpr : public Expr {
public:
    enum class Op { PLUS, MINUS, NOT };
    Op op;
    std::unique_ptr<Expr> operand;
    UnaryExpr(Op o, std::unique_ptr<Expr> e)
        : op(o), operand(std::move(e)) {}
    void accept(ASTVisitor& v) override;
};

class Identifier : public Expr {
public:
    std::string name;
    explicit Identifier(const std::string& n) : name(n) {}
    void accept(ASTVisitor& v) override;
};

class Number : public Expr {
public:
    int value = 0;
    explicit Number(int v) : value(v) {}
    void accept(ASTVisitor& v) override;
};

class FuncCall : public Expr {
public:
    std::string name;
    std::vector<std::unique_ptr<Expr>> arguments;
    FuncCall(const std::string& n, std::vector<std::unique_ptr<Expr>> args)
        : name(n), arguments(std::move(args)) {}
    void accept(ASTVisitor& v) override;
};

// Statements
class Block : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
    Block() = default;
    void accept(ASTVisitor& v) override;
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
    IfStmt(std::unique_ptr<Expr> c, std::unique_ptr<Stmt> t, std::unique_ptr<Stmt> e = nullptr)
        : condition(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {}
    void accept(ASTVisitor& v) override;
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    WhileStmt(std::unique_ptr<Expr> c, std::unique_ptr<Stmt> b)
        : condition(std::move(c)), body(std::move(b)) {}
    void accept(ASTVisitor& v) override;
};

class BreakStmt : public Stmt {
public:
    using Stmt::Stmt;
    void accept(ASTVisitor& v) override;
};

class ContinueStmt : public Stmt {
public:
    using Stmt::Stmt;
    void accept(ASTVisitor& v) override;
};

class ReturnStmt : public Stmt {
public:
    std::unique_ptr<Expr> value;
    explicit ReturnStmt(std::unique_ptr<Expr> v = nullptr) : value(std::move(v)) {}
    void accept(ASTVisitor& v) override;
};

class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;
    explicit ExprStmt(std::unique_ptr<Expr> e) : expression(std::move(e)) {}
    void accept(ASTVisitor& v) override;
};

class AssignStmt : public Stmt {
public:
    std::string name;
    std::unique_ptr<Expr> value;
    AssignStmt(const std::string& n, std::unique_ptr<Expr> v) : name(n), value(std::move(v)) {}
    void accept(ASTVisitor& v) override;
};

class EmptyStmt : public Stmt {
public:
    using Stmt::Stmt;
    void accept(ASTVisitor& v) override;
};

// Declarations
class VarDecl : public Stmt {
public:
    std::string name;
    std::unique_ptr<Expr> initExpr;
    bool isGlobal = false;
    VarDecl(const std::string& n, std::unique_ptr<Expr> i, bool g = false)
        : name(n), initExpr(std::move(i)), isGlobal(g) {}
    void accept(ASTVisitor& v) override;
};

class ConstDecl : public Stmt {
public:
    std::string name;
    std::unique_ptr<Expr> initExpr;
    bool isGlobal = false;
    ConstDecl(const std::string& n, std::unique_ptr<Expr> i, bool g = false)
        : name(n), initExpr(std::move(i)), isGlobal(g) {}
    void accept(ASTVisitor& v) override;
};

// Parameter
class Param : public ASTNode {
public:
    std::string name;
    explicit Param(const std::string& n) : name(n) {}
    void accept(ASTVisitor& v) override;
};

// Function definition
class FuncDef : public ASTNode {
public:
    Type* returnType;
    std::string name;
    std::vector<std::unique_ptr<Param>> params;
    std::unique_ptr<Block> body;
    FuncDef(Type* rt, const std::string& n, std::vector<std::unique_ptr<Param>> p, std::unique_ptr<Block> b)
        : returnType(rt), name(n), params(std::move(p)), body(std::move(b)) {}
    void accept(ASTVisitor& v) override;
};

// Compilation unit
class CompUnit : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> elements;
    CompUnit() = default;
    void accept(ASTVisitor& v) override;
};

// AST Visitor
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void visit(CompUnit* node) = 0;
    virtual void visit(VarDecl* node) = 0;
    virtual void visit(ConstDecl* node) = 0;
    virtual void visit(FuncDef* node) = 0;
    virtual void visit(Param* node) = 0;
    virtual void visit(Block* node) = 0;
    virtual void visit(IfStmt* node) = 0;
    virtual void visit(WhileStmt* node) = 0;
    virtual void visit(BreakStmt* node) = 0;
    virtual void visit(ContinueStmt* node) = 0;
    virtual void visit(ReturnStmt* node) = 0;
    virtual void visit(ExprStmt* node) = 0;
    virtual void visit(AssignStmt* node) = 0;
    virtual void visit(EmptyStmt* node) = 0;
    virtual void visit(BinaryExpr* node) = 0;
    virtual void visit(UnaryExpr* node) = 0;
    virtual void visit(Identifier* node) = 0;
    virtual void visit(Number* node) = 0;
    virtual void visit(FuncCall* node) = 0;
};

// Inline BinaryExpr::opFromString
inline BinaryExpr::Op BinaryExpr::opFromString(const std::string& s) {
    if (s == "+")  return Op::ADD;
    if (s == "-")  return Op::SUB;
    if (s == "*")  return Op::MUL;
 if (s == "/")  return Op::DIV;
    if (s == "%")  return Op::MOD;
    if (s == "==") return Op::EQ;
    if (s == "!=") return Op::NE;
    if (s == "<")  return Op::LT;
    if (s == "<=") return Op::LE;
    if (s == ">")  return Op::GT;
    if (s == ">=") return Op::GE;
    if (s == "&&") return Op::AND;
    if (s == "||") return Op::OR;
    return Op::ADD; // fallback
}

// Inline accept implementations
inline void BinaryExpr::accept(ASTVisitor& v)   { v.visit(this); }
inline void UnaryExpr::accept(ASTVisitor& v)    { v.visit(this); }
inline void Identifier::accept(ASTVisitor& v)   { v.visit(this); }
inline void Number::accept(ASTVisitor& v)       { v.visit(this); }
inline void FuncCall::accept(ASTVisitor& v)     { v.visit(this); }
inline void Block::accept(ASTVisitor& v)        { v.visit(this); }
inline void IfStmt::accept(ASTVisitor& v)       { v.visit(this); }
inline void WhileStmt::accept(ASTVisitor& v)    { v.visit(this); }
inline void BreakStmt::accept(ASTVisitor& v)    { v.visit(this); }
inline void ContinueStmt::accept(ASTVisitor& v) { v.visit(this); }
inline void ReturnStmt::accept(ASTVisitor& v)   { v.visit(this); }
inline void ExprStmt::accept(ASTVisitor& v)     { v.visit(this); }
inline void AssignStmt::accept(ASTVisitor& v)   { v.visit(this); }
inline void EmptyStmt::accept(ASTVisitor& v)    { v.visit(this); }
inline void VarDecl::accept(ASTVisitor& v)      { v.visit(this); }
inline void ConstDecl::accept(ASTVisitor& v)    { v.visit(this); }
inline void Param::accept(ASTVisitor& v)        { v.visit(this); }
inline void FuncDef::accept(ASTVisitor& v)      { v.visit(this); }
inline void CompUnit::accept(ASTVisitor& v)     { v.visit(this); }

} // namespace toyc

#endif