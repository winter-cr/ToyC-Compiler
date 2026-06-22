#include "ConstExprEvaluator.h"
#include "SemanticAnalyzer.h"
#include <stdexcept>

namespace semantic {

// AST节点类型定义（临时，应与A的AST一致）
namespace ast {
    enum ExprKind {
        NUMBER,
        ID,
        UNARY_OP,
        BINARY_OP
    };
    struct Expr {
        ExprKind kind;
        int line;
        // 简化：实际应有union或继承结构
    };
    struct NumberExpr : Expr {
        int value;
        NumberExpr(int v) : kind(NUMBER), value(v) {}
    };
    struct IdentifierExpr : Expr {
        std::string name;
        IdentifierExpr(std::string n) : kind(ID), name(std::move(n)) {}
    };
    struct UnaryExpr : Expr {
        std::string op;
        Expr* operand;
        UnaryExpr(std::string o, Expr* e) : kind(UNARY_OP), op(std::move(o)), operand(e) {}
    };
    struct BinaryExpr : Expr {
        std::string op;
        Expr* left;
        Expr* right;
        BinaryExpr(std::string o, Expr* l, Expr* r)
            : kind(BINARY_OP), op(std::move(o)), left(l), right(r) {}
    };
} // namespace ast

std::optional<int> ConstExprEvaluator::evaluate(ast::Expr* expr) {
    if (!expr) return std::nullopt;

    switch (expr->kind) {
        case ast::NUMBER: {
            auto num = static_cast<ast::NumberExpr*>(expr);
            return num->value;
        }

        case ast::ID: {
            auto id = static_cast<ast::IdentifierExpr*>(expr);
            return evaluateIdentifier(id->name, expr->line);
        }

        case ast::UNARY_OP: {
            auto unary = static_cast<ast::UnaryExpr*>(expr);
            auto operand = evaluate(unary->operand);
            return evaluateUnary(unary->op, operand, expr->line);
        }

        case ast::BINARY_OP: {
            auto bin = static_cast<ast::BinaryExpr*>(expr);
            auto left = evaluate(bin->left);
            // 短路逻辑：对于&&和||，可能不需要求值右子树
            bool skipRight = false;
            if (bin->op == "&&" || bin->op == "||") {
                bool shortResult;
                if (shouldShortCircuit(bin->op, left, shortResult)) {
                    return shortResult ? 1 : 0;  // 短路，返回结果
                }
            }
            auto right = evaluate(bin->right);
            return evaluateBinary(bin->op, left, right, expr->line);
        }

        default:
            return std::nullopt;
    }
}

std::optional<int> ConstExprEvaluator::evaluateIdentifier(const std::string& name, int line) {
    // 查找常量
    Symbol* sym = analyzer_->lookupGlobal(name);
    if (!sym) {
        // 未声明的常量 - 这里不报错，留给语义分析阶段处理
        return std::nullopt;
    }
    if (!sym->isConst) {
        // 非常量 - 不能用于常量表达式
        return std::nullopt;
    }
    return sym->getConstValue();  // 返回常量值
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
        if (r == 0) {
            // 除零错误 - 需要报告
            // 这里暂时返回nullopt，由调用者处理
            return std::nullopt;
        }
        return l / r;  // 整数除法
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

    // 逻辑运算（非短路情况，短路已在evaluate处理）
    if (op == "&&") return (l != 0 && r != 0) ? 1 : 0;
    if (op == "||") return (l != 0 || r != 0) ? 1 : 0;

    return std::nullopt;
}

bool ConstExprEvaluator::shouldShortCircuit(const std::string& op, std::optional<int> left, bool& result) {
    if (!left.has_value()) return false;  // 左值未知，不能短路

    if (op == "&&") {
        if (left.value() == 0) {
            result = 0;
            return true;  // 短路，结果是0
        }
    } else if (op == "||") {
        if (left.value() != 0) {
            result = 1;
            return true;  // 短路，结果是1
        }
    }
    return false;  // 不能短路，需要继续求值右子树
}

} // namespace semantic
