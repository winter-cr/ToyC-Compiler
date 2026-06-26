#include "semantic/SemanticAnalyzer.h"
#include "semantic/ConstExprEvaluator.h"
#include <optional>
#include <string>
#include <memory>

namespace toyc {

SemanticAnalyzer::SemanticAnalyzer() {}

bool SemanticAnalyzer::analyze(CompUnit* root) {
    if (!root) return false;
    errors_.clear();
    hasMain_ = false;
    loopDepth_ = 0;
    currentFunction_ = nullptr;
    root->accept(*this);
    return !hasErrors();
}

bool SemanticAnalyzer::hasErrors() const {
    return !errors_.empty();
}

const std::vector<SemanticError>& SemanticAnalyzer::errors() const {
    return errors_;
}

Symbol* SemanticAnalyzer::lookupGlobal(const std::string& name) const {
    return const_cast<SymbolTable&>(symTable_).lookupGlobal(name);
}

Symbol* SemanticAnalyzer::lookupLocal(const std::string& name) const {
    return const_cast<SymbolTable&>(symTable_).lookupCurrentScope(name);
}

bool SemanticAnalyzer::getConstantValue(const std::string& name, int* outValue) const {
    Symbol* sym = lookupGlobal(name);
    if (!sym) sym = lookupLocal(name);
    if (!sym || !sym->isConst || !sym->constValue.has_value()) return false;
    if (outValue) *outValue = sym->constValue.value();
    return true;
}

void SemanticAnalyzer::error(int line, int col, SemanticErrorCode code,
                              const std::string& msg) {
    errors_.push_back({line, col, code, msg});
}

Symbol* SemanticAnalyzer::declareVar(const std::string& name, SymbolKind kind,
                                      int line, bool isGlobal,
                                      std::optional<int> constVal) {
    auto sym = std::make_unique<Symbol>(name, kind, line, isGlobal);
    sym->isConst = (kind == SymbolKind::GLOBAL_CONST ||
                    kind == SymbolKind::LOCAL_CONST);
    if (constVal.has_value()) {
        sym->constValue = constVal;
    }
    if (kind == SymbolKind::FUNCTION) {
        sym->isConst = false;
    }
    Symbol* raw = sym.get();
    if (!symTable_.insert(std::move(sym))) {
        Symbol* existing = symTable_.lookupCurrentScope(name);
        if (existing) {
            return nullptr;
        }
    }
    return raw;
}

Symbol* SemanticAnalyzer::lookupSymbol(const std::string& name) {
    return symTable_.lookup(name);
}

void SemanticAnalyzer::checkExpr(const Expr* expr) {
    if (expr) {
        expr->accept(*this);
    }
}

std::optional<int> SemanticAnalyzer::evaluateConstExpr(Expr* expr) {
    ConstExprEvaluator evaluator(symTable_);
    return evaluator.evaluate(expr);
}

std::string SemanticAnalyzer::errorMessage(SemanticErrorCode code) {
    switch (code) {
        case SemanticErrorCode::ERR_UNDECLARED_IDENT:
            return "undeclared identifier";
        case SemanticErrorCode::ERR_REDEFINED_SYMBOL:
            return "redefined symbol";
        case SemanticErrorCode::ERR_USE_BEFORE_DECL:
            return "use before declaration";
        case SemanticErrorCode::ERR_CONST_REASSIGN:
            return "cannot reassign constant";
        case SemanticErrorCode::ERR_CONST_INIT_UNDEFINED:
            return "constant initializer undefined";
        case SemanticErrorCode::ERR_CONST_EXPR_NON_CONST:
            return "constant expression not compile-time evaluable";
        case SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE:
            return "invalid main function signature";
        case SemanticErrorCode::ERR_MISSING_RETURN:
            return "missing return statement";
        case SemanticErrorCode::ERR_RETURN_TYPE_MISMATCH:
            return "return type mismatch";
        case SemanticErrorCode::ERR_VOID_FUNC_CALL_EXPR:
            return "void function used as expression";
        case SemanticErrorCode::ERR_WRONG_ARG_COUNT:
            return "wrong number of arguments";
        case SemanticErrorCode::ERR_BREAK_OUTSIDE_LOOP:
            return "break outside loop";
        case SemanticErrorCode::ERR_CONTINUE_OUTSIDE_LOOP:
            return "continue outside loop";
        case SemanticErrorCode::ERR_DIVIDE_BY_ZERO:
            return "division by zero";
    }
    return "unknown semantic error";
}

// =====================================================================
// Visitor implementations ¡ª all take const& per ASTVisitor interface
// =====================================================================

void SemanticAnalyzer::visit(const CompUnit& node) {
    for (auto& elem : node.elements) {
        std::visit([this](auto& ptr) {
            if (ptr) ptr->accept(*this);
        }, elem);
    }
    if (!hasMain_) {
        error(0, 0, SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE,
              "program must contain int main(void) function");
    }
}

void SemanticAnalyzer::visit(const VarDecl& node) {
    if (node.isGlobal) {
        std::optional<int> constVal = std::nullopt;
        if (node.initExpr) {
            constVal = evaluateConstExpr(node.initExpr.get());
        }
        auto* sym = declareVar(node.name, SymbolKind::GLOBAL_VAR,
                               node.line, true, constVal);
        if (!sym) {
            error(node.line, node.column,
                  SemanticErrorCode::ERR_REDEFINED_SYMBOL,
                  "redefinition of global variable '" + node.name + "'");
        }
    } else {
        std::optional<int> constVal = std::nullopt;
        if (node.initExpr) {
            constVal = evaluateConstExpr(node.initExpr.get());
        }
        auto* sym = declareVar(node.name, SymbolKind::LOCAL_VAR,
                               node.line, false, constVal);
        if (!sym) {
            error(node.line, node.column,
                  SemanticErrorCode::ERR_REDEFINED_SYMBOL,
                  "redefinition of '" + node.name + "' in current scope");
        }
        if (node.initExpr) {
            checkExpr(node.initExpr.get());
        }
    }
}

void SemanticAnalyzer::visit(const ConstDecl& node) {
    if (node.isGlobal) {
        std::optional<int> constVal = std::nullopt;
        if (node.initExpr) {
            constVal = evaluateConstExpr(node.initExpr.get());
            if (!constVal.has_value()) {
                error(node.line, node.column,
                      SemanticErrorCode::ERR_CONST_INIT_UNDEFINED,
                      "constant '" + node.name +
                      "' initializer must be a compile-time constant expression");
                return;
            }
        } else {
            error(node.line, node.column,
                  SemanticErrorCode::ERR_CONST_INIT_UNDEFINED,
                  "global constant '" + node.name +
                  "' must have an initializer");
            return;
        }
        auto* sym = declareVar(node.name, SymbolKind::GLOBAL_CONST,
                               node.line, true, constVal);
        if (!sym) {
            error(node.line, node.column,
                  SemanticErrorCode::ERR_REDEFINED_SYMBOL,
                  "redefinition of global constant '" + node.name + "'");
        }
    } else {
        bool constOk = true;
        std::optional<int> constVal = std::nullopt;
        if (node.initExpr) {
            constVal = evaluateConstExpr(node.initExpr.get());
            if (!constVal.has_value()) {
                constOk = false;
            }
        }
        if (!constOk) {
            error(node.line, node.column,
                  SemanticErrorCode::ERR_CONST_EXPR_NON_CONST,
                  "constant '" + node.name +
                  "' initializer must be a compile-time constant expression");
            return;
        }
        auto* sym = declareVar(node.name, SymbolKind::LOCAL_CONST,
                               node.line, false, constVal);
        if (!sym) {
            error(node.line, node.column,
                  SemanticErrorCode::ERR_REDEFINED_SYMBOL,
                  "redefinition of '" + node.name + "' in current scope");
        }
        if (node.initExpr) {
            checkExpr(node.initExpr.get());
        }
    }
}

void SemanticAnalyzer::visit(const FuncDef& node) {
    if (node.name == "main") {
        if (!node.returnType || !node.returnType->is_int()) {
            error(node.line, node.column,
                  SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE,
                  "main function must return int");
        }
        if (!node.params.empty()) {
            error(node.line, node.column,
                  SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE,
                  "main function must have no parameters");
        }
        hasMain_ = true;
    }

    Symbol* existing = symTable_.lookupCurrentScope(node.name);
    if (existing) {
        error(node.line, node.column,
              SemanticErrorCode::ERR_REDEFINED_SYMBOL,
              "redefinition of function '" + node.name + "'");
        return;
    }

    auto funcSym = std::make_unique<Symbol>(node.name, SymbolKind::FUNCTION,
                                             node.line, true);
    funcSym->returnsInt = node.returnType && node.returnType->is_int();
    funcSym->paramCount = static_cast<int>(node.params.size());
    symTable_.insert(std::move(funcSym));

    currentFunction_ = const_cast<FuncDef*>(&node);

    symTable_.pushScope();

    for (auto& param : node.params) {
        if (symTable_.lookupCurrentScope(param->name)) {
            error(param->line, param->column,
                  SemanticErrorCode::ERR_REDEFINED_SYMBOL,
                  "redefinition of parameter '" + param->name + "'");
        } else {
            auto pSym = std::make_unique<Symbol>(param->name, SymbolKind::PARAM,
                                                  param->line, false);
            symTable_.insert(std::move(pSym));
        }
    }

    if (node.body) {
        node.body->accept(*this);
    }

    if (node.returnType && node.returnType->is_int()) {
        if (!checkReturnPaths(node.body.get(), true)) {
            error(node.line, node.column,
                  SemanticErrorCode::ERR_MISSING_RETURN,
                  "function '" + node.name +
                  "' may not return a value on every path");
        }
    }

    symTable_.popScope();
    currentFunction_ = nullptr;
}

void SemanticAnalyzer::visit(const Param& /*node*/) {
    // Parameters handled in FuncDef visitor
}

void SemanticAnalyzer::visit(const Block& node) {
    symTable_.pushScope();
    for (auto& stmt : node.statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
    symTable_.popScope();
}

void SemanticAnalyzer::visit(const IfStmt& node) {
    if (node.cond) {
        checkExpr(node.cond.get());
    }
    if (node.then) {
        node.then->accept(*this);
    }
    if (node.else_) {
        node.else_->accept(*this);
    }
}

void SemanticAnalyzer::visit(const WhileStmt& node) {
    if (node.cond) {
        checkExpr(node.cond.get());
    }
    loopDepth_++;
    if (node.body) {
        node.body->accept(*this);
    }
    loopDepth_--;
}

void SemanticAnalyzer::visit(const BreakStmt& node) {
    if (loopDepth_ == 0) {
        error(node.line, node.column,
              SemanticErrorCode::ERR_BREAK_OUTSIDE_LOOP,
              "break statement outside of loop");
    }
}

void SemanticAnalyzer::visit(const ContinueStmt& node) {
    if (loopDepth_ == 0) {
        error(node.line, node.column,
              SemanticErrorCode::ERR_CONTINUE_OUTSIDE_LOOP,
              "continue statement outside of loop");
    }
}

void SemanticAnalyzer::visit(const ReturnStmt& node) {
    if (!currentFunction_) return;

    if (node.value) {
        if (currentFunction_->returnType->is_void()) {
            error(node.line, node.column,
                  SemanticErrorCode::ERR_RETURN_TYPE_MISMATCH,
                  "void function should not return a value");
        }
        checkExpr(node.value.get());
    } else {
        if (currentFunction_->returnType->is_int()) {
            error(node.line, node.column,
                  SemanticErrorCode::ERR_RETURN_TYPE_MISMATCH,
                  "non-void function must return a value");
        }
    }
}

void SemanticAnalyzer::visit(const ExprStmt& node) {
    if (node.expr) {
        checkExpr(node.expr.get());
    }
}

void SemanticAnalyzer::visit(const AssignStmt& node) {
    if (node.lvalue) {
        Symbol* sym = lookupSymbol(node.lvalue->name);
        if (sym) {
            if (sym->isConst) {
                error(node.line, node.column,
                      SemanticErrorCode::ERR_CONST_REASSIGN,
                      "cannot reassign constant '" + node.lvalue->name + "'");
            }
            sym->isUsed = true;
        }
    }
    if (node.value) {
        checkExpr(node.value.get());
    }
}

void SemanticAnalyzer::visit(const BinaryExpr& node) {
    if (node.left) {
        node.left->accept(*this);
    }
    if (node.right) {
        node.right->accept(*this);
    }
    if ((node.op == BinaryOp::Div ||
         node.op == BinaryOp::Mod) && node.right) {
        auto rightVal = evaluateConstExpr(node.right.get());
        if (rightVal.has_value() && rightVal.value() == 0) {
            error(node.line, node.column,
                  SemanticErrorCode::ERR_DIVIDE_BY_ZERO,
                  "division by zero");
        }
    }
}

void SemanticAnalyzer::visit(const UnaryExpr& node) {
    if (node.operand) {
        node.operand->accept(*this);
    }
}

void SemanticAnalyzer::visit(const Identifier& node) {
    Symbol* sym = lookupSymbol(node.name);
    if (!sym) {
        error(node.line, node.column,
              SemanticErrorCode::ERR_UNDECLARED_IDENT,
              "undeclared identifier '" + node.name + "'");
    } else {
        sym->isUsed = true;
    }
}

void SemanticAnalyzer::visit(const Number& /*node*/) {
    // Literals are always valid
}

void SemanticAnalyzer::visit(const FuncCall& node) {
    Symbol* funcSym = symTable_.lookupGlobal(node.name);
    if (!funcSym || funcSym->kind != SymbolKind::FUNCTION) {
        error(node.line, node.column,
              SemanticErrorCode::ERR_UNDECLARED_IDENT,
              "undefined function '" + node.name + "'");
        return;
    }
    int actualArgs = static_cast<int>(node.args.size());
    if (actualArgs != funcSym->paramCount) {
        error(node.line, node.column,
              SemanticErrorCode::ERR_WRONG_ARG_COUNT,
              "function '" + node.name + "' expects " +
              std::to_string(funcSym->paramCount) + " arguments, got " +
              std::to_string(actualArgs));
    }
    for (auto& arg : node.args) {
        if (arg) {
            arg->accept(*this);
        }
    }
    if (!funcSym->returnsInt && !node.isStatement) {
        if (!currentFunction_ || currentFunction_->returnType->is_int()) {
            error(node.line, node.column,
                  SemanticErrorCode::ERR_VOID_FUNC_CALL_EXPR,
                  "void function '" + node.name +
                  "' used in expression context");
        }
    }
}

bool SemanticAnalyzer::checkReturnPaths(const Block* block, bool expectReturn) {
    if (!block) return !expectReturn;

    for (auto& stmt : block->statements) {
        if (!stmt) continue;
        if (stmtGuaranteesReturn(stmt.get(), expectReturn)) {
            return true;
        }
    }
    return false;
}

bool SemanticAnalyzer::stmtGuaranteesReturn(const Stmt* stmt, bool expectReturn) {
    if (!stmt) return false;

    if (auto* ret = dynamic_cast<const ReturnStmt*>(stmt)) {
        return expectReturn ? ret->value != nullptr : ret->value == nullptr;
    }

    if (auto* block = dynamic_cast<const Block*>(stmt)) {
        return checkReturnPaths(block, expectReturn);
    }

    if (auto* ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        return ifStmt->else_ &&
               stmtGuaranteesReturn(ifStmt->then.get(), expectReturn) &&
               stmtGuaranteesReturn(ifStmt->else_.get(), expectReturn);
    }

    if (auto* whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        return exprIsConstantNonZero(whileStmt->cond.get()) &&
               stmtGuaranteesReturn(whileStmt->body.get(), expectReturn);
    }

    return false;
}

bool SemanticAnalyzer::exprIsConstantNonZero(const Expr* expr) {
    if (!expr) return false;
    auto* mutableExpr = const_cast<Expr*>(expr);
    auto value = evaluateConstExpr(mutableExpr);
    return value.has_value() && value.value() != 0;
}
} // namespace toyc
