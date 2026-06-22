#pragma once

#include <string>
#include <optional>

// 前向声明 - AST节点由A定义
namespace ast {
class Type;
class Expr;
class Stmt;
class ASTNode;
}

// 类型定义（简化版，目前只支持int）
enum class BasicType {
    INT,
    VOID
};

// 符号种类
enum class SymbolKind {
    GLOBAL_VAR,
    GLOBAL_CONST,
    LOCAL_VAR,
    LOCAL_CONST,
    PARAM,
    FUNCTION
};

// 语义错误码
enum class SemanticErrorCode {
    // 标识符相关
    ERR_UNDECLARED_IDENT,
    ERR_REDEFINED_SYMBOL,
    ERR_USE_BEFORE_DECL,

    // 常量相关
    ERR_CONST_REASSIGN,
    ERR_CONST_INIT_UNDEFINED,
    ERR_CONST_EXPR_NON_CONST,

    // 函数相关
    ERR_INVALID_MAIN_SIGNATURE,
    ERR_MISSING_RETURN,
    ERR_RETURN_TYPE_MISMATCH,
    ERR_VOID_FUNC_CALL_EXPR,
    ERR_WRONG_ARG_COUNT,

    // 语句相关
    ERR_BREAK_OUTSIDE_LOOP,
    ERR_CONTINUE_OUTSIDE_LOOP,

    // 其他
    ERR_DIVIDE_BY_ZERO
};

// 语义错误信息结构
struct SemanticError {
    int line;
    int column;
    SemanticErrorCode code;
    std::string message;

    SemanticError(int l, int c, SemanticErrorCode err_code, std::string msg)
        : line(l), column(c), code(err_code), message(std::move(msg)) {}
};

// 符号类
class Symbol {
public:
    SymbolKind kind;
    std::string name;
    BasicType type;             // 目前只支持INT或VOID
    bool isConst;               // 是否为const
    std::optional<int> constValue;  // 常量值（仅当isConst有效）
    int line;                   // 声明行号
    int offset;                 // 栈帧偏移（-1表示未分配）
    bool isGlobal;              // 是否全局符号
    bool isUsed;                // 死变量检测用（D模块使用）

    Symbol(SymbolKind k, std::string n, BasicType t, bool is_const, int l)
        : kind(k), name(std::move(n)), type(t), isConst(is_const), line(l),
          offset(-1), isGlobal(false), isUsed(false) {
        if (isConst) {
            constValue = std::nullopt;  // 初始化时未知，求值后填充
        }
    }

    // 判断是否为常量（供优化使用）
    bool isConstant() const { return isConst; }

    // 获取常量值（如果已知）
    std::optional<int> getConstValue() const {
        return isConst ? constValue : std::nullopt;
    }
};

// 作用域层级
struct Scope {
    int level;  // 嵌套深度，0=全局
    std::unordered_map<std::string, std::unique_ptr<Symbol>> symbols;

    explicit Scope(int l) : level(l) {}
};

}  // namespace semantic
