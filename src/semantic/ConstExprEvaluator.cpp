#include "semantic/ConstExprEvaluator.h"
#include "semantic/Symbol.h"


namespace toyc {

std::optional<int> ConstExprEvaluator::evaluate(Expr* expr) {
    if (!expr) return std::nullopt;
    if (auto* n = dynamic_cast<Number*>(expr)) return evalNumber(n);
    if (auto* id = dynamic_cast<Identifier*>(expr)) return evalIdentifier(id);
    if (auto* bin = dynamic_cast<BinaryExpr*>(expr)) return evalBinary(bin);
    if (auto* un = dynamic_cast<UnaryExpr*>(expr)) return evalUnary(un);
    return std::nullopt;
}

std::optional<int> ConstExprEvaluator::evalNumber(Number* node) {
    return node->value;
}

std::optional<int> ConstExprEvaluator::evalIdentifier(Identifier* node) {
    Symbol* sym = symTable_.lookup(node->name);
    if (sym && sym->isConst && sym->constValue.has_value()) {
        return sym->constValue;
    }
    return std::nullopt;
}

std::optional<int> ConstExprEvaluator::evalBinary(BinaryExpr* node) {
    if (node->op == BinaryOp::And) {
        auto left = evaluate(node->left.get());
        if (!left.has_value()) return std::nullopt;
        if (left.value() == 0) return 0;
        return evaluate(node->right.get());
    }
    if (node->op == BinaryOp::Or) {
        auto left = evaluate(node->left.get());
        if (!left.has_value()) return std::nullopt;
        if (left.value() != 0) return 1;
        return evaluate(node->right.get());
    }

    auto left = evaluate(node->left.get());
    auto right = evaluate(node->right.get());
    if (!left.has_value() || !right.has_value()) return std::nullopt;

    int l = left.value();
    int r = right.value();

    switch (node->op) {
        case BinaryOp::Add: return l + r;
        case BinaryOp::Sub: return l - r;
        case BinaryOp::Mul: return l * r;
        case BinaryOp::Div:
            if (r == 0) return std::nullopt;
            return l / r;
        case BinaryOp::Mod:
            if (r == 0) return std::nullopt;
            return l % r;
        case BinaryOp::Eq:  return (l == r) ? 1 : 0;
        case BinaryOp::Ne:  return (l != r) ? 1 : 0;
        case BinaryOp::Lt:  return (l < r)  ? 1 : 0;
        case BinaryOp::Le:  return (l <= r) ? 1 : 0;
        case BinaryOp::Gt:  return (l > r)  ? 1 : 0;
        case BinaryOp::Ge:  return (l >= r) ? 1 : 0;
        default: return std::nullopt;
    }
}

std::optional<int> ConstExprEvaluator::evalUnary(UnaryExpr* node) {
    auto val = evaluate(node->operand.get());
    if (!val.has_value()) return std::nullopt;

    switch (node->op) {
        case UnaryOp::Plus:  return val.value();
        case UnaryOp::Minus: return -val.value();
        case UnaryOp::Not:   return (val.value() == 0) ? 1 : 0;
        default: return std::nullopt;
    }
}

} // namespace toyc
