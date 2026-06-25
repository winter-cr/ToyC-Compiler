#include "ast/AST.h"

namespace toyc {

const char* binary_op_name(BinaryOp op) {
    switch (op) {
    case BinaryOp::Add: return "+";
    case BinaryOp::Sub: return "-";
    case BinaryOp::Mul: return "*";
    case BinaryOp::Div: return "/";
    case BinaryOp::Mod: return "%";
    case BinaryOp::Lt: return "<";
    case BinaryOp::Gt: return ">";
    case BinaryOp::Le: return "<=";
    case BinaryOp::Ge: return ">=";
    case BinaryOp::Eq: return "==";
    case BinaryOp::Ne: return "!=";
    case BinaryOp::And: return "&&";
    case BinaryOp::Or: return "||";
    }
    return "?";
}

const char* unary_op_name(UnaryOp op) {
    switch (op) {
    case UnaryOp::Plus: return "+";
    case UnaryOp::Minus: return "-";
    case UnaryOp::Not: return "!";
    }
    return "?";
}

void BinaryExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void UnaryExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void Number::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void Identifier::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void FuncCall::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void VarDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ConstDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void Param::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void Block::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void FuncDef::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ExprStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void AssignStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void IfStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void WhileStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void BreakStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ContinueStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ReturnStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void CompUnit::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

} // namespace toyc
