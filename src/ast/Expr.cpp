#include "ast/Expr.h"
#include "ast/AstVisitor.h"

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

void BinaryExpr::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void UnaryExpr::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void IntLiteral::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void IdentifierExpr::accept(AstVisitor& visitor) const { visitor.visit(*this); }
void CallExpr::accept(AstVisitor& visitor) const { visitor.visit(*this); }
