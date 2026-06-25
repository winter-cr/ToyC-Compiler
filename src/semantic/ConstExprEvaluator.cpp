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
    using Op = BinaryExpr::Op;

    if (node->op == Op::AND) {
        auto left = evaluate(node->left.get());
        if (!left.has_value()) return std::nullopt;
        if (left.value() == 0) return 0;
        return evaluate(node->right.get());
    }
    if (node->op == Op::OR) {
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
        case Op::ADD: return l + r;
        case Op::SUB: return l - r;
        case Op::MUL: return l * r;
        case Op::DIV:
            if (r == 0) return std::nullopt;
            return l / r;
        case Op::MOD:
            if (r == 0) return std::nullopt;
            return l % r;
        case Op::EQ:  return (l == r) ? 1 : 0;
        case Op::NE:  return (l != r) ? 1 : 0;
        case Op::LT:  return (l < r)  ? 1 : 0;
        case Op::LE:  return (l <= r) ? 1 : 0;
        case Op::GT:  return (l > r)  ? 1 : 0;
        case Op::GE:  return (l >= r) ? 1 : 0;
        default: return std::nullopt;
    }
}

std::optional<int> ConstExprEvaluator::evalUnary(UnaryExpr* node) {
    auto val = evaluate(node->operand.get());
    if (!val.has_value()) return std::nullopt;

    switch (node->op) {
        case UnaryExpr::Op::PLUS:  return val.value();
        case UnaryExpr::Op::MINUS: return -val.value();
        case UnaryExpr::Op::NOT:   return (val.value() == 0) ? 1 : 0;
        default: return std::nullopt;
    }
}

} // namespace toyc
