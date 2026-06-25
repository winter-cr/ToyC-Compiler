#ifndef TOYC_ERRORS_H
#define TOYC_ERRORS_H

#include <string>
#include <utility>

namespace toyc {

enum class SemanticErrorCode {
    ERR_UNDECLARED_IDENT,
    ERR_REDEFINED_SYMBOL,
    ERR_USE_BEFORE_DECL,
    ERR_CONST_REASSIGN,
    ERR_CONST_INIT_UNDEFINED,
    ERR_CONST_EXPR_NON_CONST,
    ERR_INVALID_MAIN_SIGNATURE,
    ERR_MISSING_RETURN,
    ERR_RETURN_TYPE_MISMATCH,
    ERR_VOID_FUNC_CALL_EXPR,
    ERR_WRONG_ARG_COUNT,
    ERR_BREAK_OUTSIDE_LOOP,
    ERR_CONTINUE_OUTSIDE_LOOP,
    ERR_DIVIDE_BY_ZERO,
};

struct SemanticError {
    int line;
    int column;
    SemanticErrorCode code;
    std::string message;

    SemanticError(int l, int col, SemanticErrorCode c, std::string msg)
        : line(l), column(col), code(c), message(std::move(msg)) {}
};

} // namespace toyc

#endif
