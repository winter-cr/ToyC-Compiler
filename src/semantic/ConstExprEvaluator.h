#ifndef TOYC_CONST_EXPR_EVALUATOR_H
#define TOYC_CONST_EXPR_EVALUATOR_H

#include "ast/AST.h"
#include "semantic/SymbolTable.h"
#include <optional>

namespace toyc {


class ConstExprEvaluator {
public:
    explicit ConstExprEvaluator(SymbolTable& symTable) : symTable_(symTable) {}

    std::optional<int> evaluate(Expr* expr);

private:
    SymbolTable& symTable_;

    std::optional<int> evalNumber(Number* node);
    std::optional<int> evalIdentifier(Identifier* node);
    std::optional<int> evalBinary(BinaryExpr* node);
    std::optional<int> evalUnary(UnaryExpr* node);
};

} // namespace toyc

#endif
