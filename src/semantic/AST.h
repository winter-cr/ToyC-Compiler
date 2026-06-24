#pragma once

#include <string>
#include <vector>
#include <memory>

namespace semantic {
namespace ast {

// 类型（简化：只支持int，函数用void表示返回类型）
struct Type {
    bool isInt;
    bool isVoid;
    explicit Type(bool int_type = true) : isInt(int_type), isVoid(!int_type) {}
    static Type* intType() { return new Type(true); }
    static Type* voidType() { return new Type(false); }
};

// ==================== 表达式 ====================
struct Expr {
    int line;
    int column;
    Type* type;
    Expr(int l, int c, Type* t) : line(l), column(c), type(t) {}
    virtual ~Expr() = default;
};

struct Number : Expr {
    int value;
    Number(int l, int c, int v) : Expr(l, c, Type::intType()), value(v) {}
};

struct Identifier : Expr {
    std::string name;
    Identifier(int l, int c, std::string n) : Expr(l, c, Type::intType()), name(std::move(n)) {}
};

struct UnaryOp : Expr {
    std::string op;
    Expr* operand;
    UnaryOp(int l, int c, std::string o, Expr* e) : Expr(l, c, Type::intType()), op(std::move(o)), operand(e) {}
};

struct BinaryOp : Expr {
    std::string op;
    Expr* left;
    Expr* right;
    BinaryOp(int l, int c, std::string o, Expr* lf, Expr* rt)
        : Expr(l, c, Type::intType()), op(std::move(o)), left(lf), right(rt) {}
};

struct FuncCall : Expr {
    std::string calleeName;
    std::vector<Expr*> arguments;
    bool isStatement;
    FuncCall(int l, int c, std::string n, std::vector<Expr*> args, bool is_stmt = false)
        : Expr(l, c, Type::intType()), calleeName(std::move(n)), arguments(std::move(args)), isStatement(is_stmt) {}
};

// ==================== 声明 ====================
struct Decl {
    int line;
    int column;
    std::string name;
    Type* type;
    Decl(int l, int c, std::string n, Type* t) : line(l), column(c), name(std::move(n)), type(t) {}
    virtual ~Decl() = default;
};

struct VarDecl : Decl {
    Expr* initExpr;
    bool isConst;
    VarDecl(int l, int c, std::string n, Type* t, Expr* init, bool is_const)
        : Decl(l, c, std::move(n), t), initExpr(init), isConst(is_const) {}
};

struct ConstDecl : Decl {
    Expr* initExpr;
    ConstDecl(int l, int c, std::string n, Type* t, Expr* init)
        : Decl(l, c, std::move(n), t), initExpr(init) {}
};

// ==================== 函数 ====================
struct FuncDef {
    int line;
    std::string name;
    Type* returnType;
    std::vector<VarDecl*> params;
    struct Block* body;
    FuncDef(int l, std::string n, Type* ret, std::vector<VarDecl*> p, Block* b)
        : line(l), name(std::move(n)), returnType(ret), params(std::move(p)), body(b) {}
};

// ==================== 语句 ====================
struct Stmt {
    int line;
    virtual ~Stmt() = default;
    explicit Stmt(int l) : line(l) {}
};

struct Assignment : Stmt {
    Expr* lvalue;
    Expr* rvalue;
    Assignment(int l, Expr* lv, Expr* rv) : Stmt(l), lvalue(lv), rvalue(rv) {}
};

struct ReturnStmt : Stmt {
    Expr* returnExpr;
    ReturnStmt(int l, Expr* e) : Stmt(l), returnExpr(e) {}
};

struct ExprStmt : Stmt {
    Expr* expr;
    ExprStmt(int l, Expr* e) : Stmt(l), expr(e) {}
};

struct IfStmt : Stmt {
    Expr* condition;
    Block* thenBranch;
    Block* elseBranch;
    IfStmt(int l, Expr* cond, Block* thenb, Block* elseb)
        : Stmt(l), condition(cond), thenBranch(thenb), elseBranch(elseb) {}
};

struct WhileStmt : Stmt {
    Expr* condition;
    Block* body;
    WhileStmt(int l, Expr* cond, Block* b) : Stmt(l), condition(cond), body(b) {}
};

struct BreakStmt : Stmt {
    explicit BreakStmt(int l) : Stmt(l) {}
};

struct ContinueStmt : Stmt {
    explicit ContinueStmt(int l) : Stmt(l) {}
};

// ==================== 块与编译单元 ====================
struct Block {
    std::vector<Stmt*> stmts;
};

struct CompUnit {
    std::vector<Decl*> globalDecls;
    std::vector<FuncDef*> functions;
    const std::vector<Decl*>& getGlobalDecls() const { return globalDecls; }
    const std::vector<FuncDef*>& getFunctions() const { return functions; }
};

// AST 根节点（简化版，不需要多态）
struct ASTNode {
    virtual ~ASTNode() = default;
};

} // namespace ast
} // namespace semantic
