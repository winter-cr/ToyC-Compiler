#pragma once

#include "Symbol.h"
#include <string>

// 语义错误码转字符串的辅助函数
namespace semantic {

// 错误码描述映射
constexpr const char* getErrorCodeString(SemanticErrorCode code) {
    switch (code) {
        // 标识符相关
        case SemanticErrorCode::ERR_UNDECLARED_IDENT:
            return "SEM_UNDECLARED_IDENT";
        case SemanticErrorCode::ERR_REDEFINED_SYMBOL:
            return "SEM_REDEFINED_SYMBOL";
        case SemanticErrorCode::ERR_USE_BEFORE_DECL:
            return "SEM_USE_BEFORE_DECL";

        // 常量相关
        case SemanticErrorCode::ERR_CONST_REASSIGN:
            return "SEM_CONST_REASSIGN";
        case SemanticErrorCode::ERR_CONST_INIT_UNDEFINED:
            return "SEM_CONST_INIT_UNDEFINED";
        case SemanticErrorCode::ERR_CONST_EXPR_NON_CONST:
            return "SEM_CONST_EXPR_NON_CONST";

        // 函数相关
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

        // 语句相关
        case SemanticErrorCode::ERR_BREAK_OUTSIDE_LOOP:
            return "SEM_BREAK_OUTSIDE_LOOP";
        case SemanticErrorCode::ERR_CONTINUE_OUTSIDE_LOOP:
            return "SEM_CONTINUE_OUTSIDE_LOOP";

        // 其他
        case SemanticErrorCode::ERR_DIVIDE_BY_ZERO:
            return "SEM_DIVIDE_BY_ZERO";

        default:
            return "SEM_UNKNOWN_ERROR";
    }
}

// 错误消息模板
std::string getErrorMessage(SemanticErrorCode code, const std::string& detail = "");

}
