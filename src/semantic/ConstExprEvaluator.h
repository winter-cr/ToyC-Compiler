#pragma once

#include <optional>
#include <string>
#include "errors.h"

namespace semantic {
namespace ast {
class Expr;
}}

namespace semantic {

class ConstExprEvaluator {
public:
    explicit ConstExprEvaluator(class SemanticAnalyzer* analyzer) : analyzer_(analyzer) {}

    struct EvalResult {
        std::optional<int> value;
        SemanticErrorCode error;  // ERR_OK (0) means no error
    };

    EvalResult evaluate(ast::Expr* expr);

private:
    class SemanticAnalyzer* analyzer_;

    EvalResult evaluateIdentifier(const std::string& name, int line);
    std::optional<int> evaluateUnary(const std::string& op, std::optional<int> operand, int line);
    std::optional<int> evaluateBinary(const std::string& op, std::optional<int> left,
                                      std::optional<int> right, int line);

    bool shouldShortCircuit(const std::string& op, std::optional<int> left, bool& result);
};

} // namespace semantic
