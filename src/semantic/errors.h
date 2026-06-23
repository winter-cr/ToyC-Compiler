#pragma once

#include "Symbol.h"
#include <string>

namespace semantic {

constexpr const char* getErrorCodeString(SemanticErrorCode code) {
    switch (code) {
        case SemanticErrorCode::ERR_UNDECLARED_IDENT:
            return "SEM_UNDECLARED_IDENT";
        case SemanticErrorCode::ERR_REDEFINED_SYMBOL:
            return "SEM_REDEFINED_SYMBOL";
        case SemanticErrorCode::ERR_USE_BEFORE_DECL:
            return "SEM_USE_BEFORE_DECL";
        case SemanticErrorCode::ERR_CONST_REASSIGN:
            return "SEM_CONST_REASSIGN";
        case SemanticErrorCode::ERR_CONST_INIT_UNDEFINED:
            return "SEM_CONST_INIT_UNDEFINED";
        case SemanticErrorCode::ERR_CONST_EXPR_NON_CONST:
            return "SEM_CONST_EXPR_NON_CONST";
        case SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE:
            return "SEM_INVALID_MAIN_SIGNATURE";
        case SemanticErrorCode::ERR_MISSING_RETURN:
            return "SEM_MISSING_RETURN";
        case SemanticErrorCode::ERR_RETURN_TYPE_MISMATCH:
            return "SEM_RETURN_TYPE_MISMATCH";
        case SemanticErrorCode::ERR_VOID_FUNC_CALL_EXPR:
            return "SEM_VOID_FUNC_CALL_EXPR";
        case SemanticErrorCode::ERR_WRONG_ARG_COUNT:
            return "SEM_WRONG_ARG_COUNT";
        case SemanticErrorCode::ERR_BREAK_OUTSIDE_LOOP:
            return "SEM_BREAK_OUTSIDE_LOOP";
        case SemanticErrorCode::ERR_CONTINUE_OUTSIDE_LOOP:
            return "SEM_CONTINUE_OUTSIDE_LOOP";
        case SemanticErrorCode::ERR_DIVIDE_BY_ZERO:
            return "SEM_DIVIDE_BY_ZERO";
        case SemanticErrorCode::ERR_OK:
            return "";
        default:
            return "SEM_UNKNOWN_ERROR";
    }
}

inline std::string getErrorMessage(SemanticErrorCode code, const std::string& detail = "") {
    switch (code) {
        case SemanticErrorCode::ERR_UNDECLARED_IDENT:
            return "use of undeclared '" + detail + "'";
        case SemanticErrorCode::ERR_REDEFINED_SYMBOL:
            return "redefinition of '" + detail + "'";
        case SemanticErrorCode::ERR_USE_BEFORE_DECL:
            return "use of '" + detail + "' before declaration";
        case SemanticErrorCode::ERR_CONST_REASSIGN:
            return "cannot assign to const variable '" + detail + "'";
        case SemanticErrorCode::ERR_CONST_INIT_UNDEFINED:
            return "const initializer is not a constant expression";
        case SemanticErrorCode::ERR_CONST_EXPR_NON_CONST:
            return "constant expression references non-const variable '" + detail + "'";
        case SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE:
            return detail.empty() ? "invalid main function" : detail;
        case SemanticErrorCode::ERR_MISSING_RETURN:
            return "non-void function must return a value on all paths";
        case SemanticErrorCode::ERR_RETURN_TYPE_MISMATCH:
            return "return type mismatch";
        case SemanticErrorCode::ERR_VOID_FUNC_CALL_EXPR:
            return "void function '" + detail + "' used in expression context";
        case SemanticErrorCode::ERR_WRONG_ARG_COUNT:
            return "wrong number of arguments for function '" + detail + "'";
        case SemanticErrorCode::ERR_BREAK_OUTSIDE_LOOP:
            return "break statement outside of loop";
        case SemanticErrorCode::ERR_CONTINUE_OUTSIDE_LOOP:
            return "continue statement outside of loop";
        case SemanticErrorCode::ERR_DIVIDE_BY_ZERO:
            return "division by zero";
        case SemanticErrorCode::ERR_OK:
            return "";
        default:
            return "unknown semantic error";
    }
}

} // namespace semantic
