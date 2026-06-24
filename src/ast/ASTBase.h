#pragma once

struct SourceLocation {
    int line = 1;
    int column = 1;
};

namespace toyc {

class ASTVisitor;
class Type;

class ASTNode {
public:
    int line;
    int column;

    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) const = 0;

protected:
    ASTNode(int line, int column) : line(line), column(column) {}
};

class Expr : public ASTNode {
public:
    Type* type;

protected:
    Expr(int line, int column, Type* type) : ASTNode(line, column), type(type) {}
};

class Stmt : public ASTNode {
protected:
    explicit Stmt(int line, int column) : ASTNode(line, column) {}
};

} // namespace toyc
