#include "ConstExprEvaluator.h"
#include "SemanticAnalyzer.h"
#include "AST.h"
#include "errors.h"
#include <stdexcept>

namespace semantic {

ConstExprEvaluator::EvalResult ConstExprEvaluator::evaluate(ast::Expr* expr) {
    if (!expr) {
        return {std::nullopt, SemanticErrorCode::ERR_CONST_INIT_UNDEFINED};
    }

    // 根据具体类型分发
    if (auto* num = dynamic_cast<ast::Number*>(expr)) {
        return {num->value, SemanticErrorCode::ERR_OK};
    }
    if (auto* id = dynamic_cast<ast::Identifier*>(expr)) {
        return evaluateIdentifier(id->name, expr->line);
    }
    if (auto* unary = dynamic_cast<ast::UnaryOp*>(expr)) {
        auto operand = evaluate(unary->operand);
        if (!operand.value.has_value()) {
            return {std::nullopt, operand.error};
        }
        auto result = evaluateUnary(unary->op, operand.value, expr->line);
        return {result, SemanticErrorCode::ERR_OK};
    }
    if (auto* bin = dynamic_cast<ast::BinaryOp*>(expr)) {
        auto left = evaluate(bin->left);
        if (!left.value.has_value()) {
            return {std::nullopt, left.error};
        }
        // 短路逻辑
        if (bin->op == "&&" || bin->op == "||") {
            bool shortResult;
            if (shouldShortCircuit(bin->op, left.value, shortResult)) {
                return {shortResult ? 1 : 0, SemanticErrorCode::ERR_OK};
            }
        }
        auto right = evaluate(bin->right);
        if (!right.value.has_value()) {
            return {std::nullopt, right.error};
        }
        auto binResult = evaluateBinary(bin->op, left.value, right.value, expr->line);
        if (!binResult.has_value()) {
            // 除零错误（/ 或 % 操作符导致）
            return {std::nullopt, SemanticErrorCode::ERR_DIVIDE_BY_ZERO};
        }
        return {binResult.value(), SemanticErrorCode::ERR_OK};
    }
    if (auto* call = dynamic_cast<ast::FuncCall*>(expr)) {
        // 函数调用不是常量表达式
        return {std::nullopt, SemanticErrorCode::ERR_CONST_INIT_UNDEFINED};
    }

    return {std::nullopt, SemanticErrorCode::ERR_CONST_INIT_UNDEFINED};
}

ConstExprEvaluator::EvalResult ConstExprEvaluator::evaluateIdentifier(const std::string& name, int line) {
    Symbol* sym = analyzer_->lookupGlobal(name);
    if (!sym) {
        return {std::nullopt, SemanticErrorCode::ERR_CONST_INIT_UNDEFINED};  // 未声明的常量
    }
    if (!sym->isConst) {
        return {std::nullopt, SemanticErrorCode::ERR_CONST_EXPR_NON_CONST};  // 非常量
    }
    auto val = sym->getConstValue();
    if (!val.has_value()) {
        return {std::nullopt, SemanticErrorCode::ERR_CONST_INIT_UNDEFINED};
    }
    return {val.value(), SemanticErrorCode::ERR_OK};
}

std::optional<int> ConstExprEvaluator::evaluateUnary(const std::string& op, std::optional<int> operand, int line) {
    if (!operand.has_value()) return std::nullopt;

    int val = operand.value();
    if (op == "+") return val;
    if (op == "-") return -val;
    if (op == "!") return val == 0 ? 1 : 0;

    return std::nullopt;
}

std::optional<int> ConstExprEvaluator::evaluateBinary(const std::string& op, std::optional<int> left,
                                                     std::optional<int> right, int line) {
    if (!left.has_value() || !right.has_value()) return std::nullopt;

    int l = left.value();
    int r = right.value();

    // 算术运算
    if (op == "+") return l + r;
    if (op == "-") return l - r;
    if (op == "*") return l * r;
    if (op == "/") {
        if (r == 0) return std::nullopt;  // 除零
        return l / r;
    }
    if (op == "%") {
        if (r == 0) return std::nullopt;
        return l % r;
    }

    // 关系运算
    if (op == "<")  return l < r ? 1 : 0;
    if (op == ">")  return l > r ? 1 : 0;
    if (op == "<=") return l <= r ? 1 : 0;
    if (op == ">=") return l >= r ? 1 : 0;
    if (op == "==") return l == r ? 1 : 0;
    if (op == "!=") return l != r ? 1 : 0;

    // 逻辑运算
    if (op == "&&") return (l != 0 && r != 0) ? 1 : 0;
    if (op == "||") return (l != 0 || r != 0) ? 1 : 0;

    return std::nullopt;
}

bool ConstExprEvaluator::shouldShortCircuit(const std::string& op, std::optional<int> left, bool& result) {
    if (!left.has_value()) return false;

    if (op == "&&") {
        if (left.value() == 0) {
            result = 0;
            return true;
        }
    } else if (op == "||") {
        if (left.value() != 0) {
            result = 1;
            return true;
        }
    }
    return false;
}

} // namespace semantic
