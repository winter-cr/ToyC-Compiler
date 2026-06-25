#ifndef TOYC_AST_BASE_H
#define TOYC_AST_BASE_H

namespace toyc {

class ASTVisitor;

class ASTNode {
public:
    int line = 0;
    int column = 0;
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& v) = 0;
};

class Expr : public ASTNode {};

class Stmt : public ASTNode {};

} // namespace toyc

#endif
