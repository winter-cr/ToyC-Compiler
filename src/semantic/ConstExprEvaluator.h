#pragma once

#include "SymbolTable.h"
#include "Symbol.h"
#include <optional>
#include <string>

namespace ast {
    class Expr;
}

namespace semantic {

// 前向声明
class SemanticAnalyzer;

// 常量表达式求值器
class ConstExprEvaluator {
public:
    explicit ConstExprEvaluator(SemanticAnalyzer* analyzer) : analyzer_(analyzer) {}

    // 求值入口，返回可选整数，失败则返回nullopt
    std::optional<int> evaluate(ast::Expr* expr);

private:
    SemanticAnalyzer* analyzer_;  // 用于查询符号（常量）

    // 访问各种表达式节点
    std::optional<int> visitNumber(/* number literal */);
    std::optional<int> visitIdentifier(const std::string& name, int line);
    std::optional<int> visitUnary(const std::string& op, std::optional<int> operand);
    std::optional<int> visitBinary(const std::string& op, std::optional<int> left, std::optional<int> right, int line);

    // 短路逻辑：&& 和 ||
    bool shouldShortCircuit(const std::string& op, std::optional<int> left, bool& result);
};

} // namespace semantic
