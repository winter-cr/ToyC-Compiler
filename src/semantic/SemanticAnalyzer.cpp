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
            return nullptr; // redefinition
        }
    }
    return raw;
}

Symbol* SemanticAnalyzer::lookupSymbol(const std::string& name) {
    return symTable_.lookup(name);
}

void SemanticAnalyzer::checkExpr(Expr* expr) {
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
// Visitor implementations
// =====================================================================

void SemanticAnalyzer::visit(CompUnit* node) {
    for (auto& elem : node->elements) {
        if (elem) {
            elem->accept(*this);
        }
    }
    if (!hasMain_) {
        error(0, 0, SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE,
              "program must contain int main(void) function");
    }
}

void SemanticAnalyzer::visit(VarDecl* node) {
    if (node->isGlobal) {
        // Global variable declaration
        std::optional<int> constVal = std::nullopt;
        if (node->initExpr) {
            constVal = evaluateConstExpr(node->initExpr.get());
        }
        auto* sym = declareVar(node->name, SymbolKind::GLOBAL_VAR,
                               node->line, true, constVal);
        if (!sym) {
            error(node->line, node->column,
                  SemanticErrorCode::ERR_REDEFINED_SYMBOL,
                  "redefinition of global variable '" + node->name + "'");
        }
    } else {
        // Local variable declaration
        std::optional<int> constVal = std::nullopt;
        if (node->initExpr) {
            constVal = evaluateConstExpr(node->initExpr.get());
        }
        auto* sym = declareVar(node->name, SymbolKind::LOCAL_VAR,
                               node->line, false, constVal);
        if (!sym) {
            error(node->line, node->column,
                  SemanticErrorCode::ERR_REDEFINED_SYMBOL,
                  "redefinition of '" + node->name + "' in current scope");
        }
        if (node->initExpr) {
            checkExpr(node->initExpr.get());
        }
    }
}

void SemanticAnalyzer::visit(ConstDecl* node) {
    if (node->isGlobal) {
        // Global constant
        std::optional<int> constVal = std::nullopt;
        if (node->initExpr) {
            constVal = evaluateConstExpr(node->initExpr.get());
            if (!constVal.has_value()) {
                error(node->line, node->column,
                      SemanticErrorCode::ERR_CONST_INIT_UNDEFINED,
                      "constant '" + node->name +
                      "' initializer must be a compile-time constant expression");
                return;
            }
        } else {
            error(node->line, node->column,
                  SemanticErrorCode::ERR_CONST_INIT_UNDEFINED,
                  "global constant '" + node->name +
                  "' must have an initializer");
            return;
        }
        auto* sym = declareVar(node->name, SymbolKind::GLOBAL_CONST,
                               node->line, true, constVal);
        if (!sym) {
            error(node->line, node->column,
                  SemanticErrorCode::ERR_REDEFINED_SYMBOL,
                  "redefinition of global constant '" + node->name + "'");
        }
    } else {
        // Local constant - still requires compile-time evaluation
        bool constOk = true;
        std::optional<int> constVal = std::nullopt;
        if (node->initExpr) {
            constVal = evaluateConstExpr(node->initExpr.get());
            if (!constVal.has_value()) {
               constOk = false;
            }
        }
        if (!constOk) {
            error(node->line, node->column,
                  SemanticErrorCode::ERR_CONST_EXPR_NON_CONST,
                  "local constant '" + node->name +
                  "' requires compile-time constant initializer");
        }
        auto* sym = declareVar(node->name, SymbolKind::LOCAL_CONST,
                               node->line, false, constVal);
        if (!sym) {
            error(node->line, node->column,
                  SemanticErrorCode::ERR_REDEFINED_SYMBOL,
                  "redefinition of '" + node->name + "' in current scope");
        }
        if (node->initExpr) {
            checkExpr(node->initExpr.get());
        }
    }
}

void SemanticAnalyzer::visit(FuncDef* node) {
    // Check main function signature
    if (node->name == "main") {
        if (!node->returnType || !node->returnType->is_int()) {
            error(node->line, node->column,
                  SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE,
                  "main function must return int");
        } else if (!node->params.empty()) {
            error(node->line, node->column,
                  SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE,
                  "main function must have no parameters (int main(void))");
        } else if (hasMain_) {
            error(node->line, node->column,
                  SemanticErrorCode::ERR_REDEFINED_SYMBOL,
                  "more than one main function defined");
        } else {
            hasMain_ = true;
        }
    }

    // Register function symbol in global scope
    {
        auto funcSym = std::make_unique<Symbol>(node->name, SymbolKind::FUNCTION,
                                                 node->line, true);
        funcSym->paramCount = static_cast<int>(node->params.size());
        funcSym->returnsInt = node->returnType && node->returnType->is_int();

        if (symTable_.lookupCurrentScope(node->name)) {
            error(node->line, node->column,
                  SemanticErrorCode::ERR_REDEFINED_SYMBOL,
                  "redefinition of function '" + node->name + "'");
            return;
        }
        symTable_.insert(std::move(funcSym));
    }

    // Enter function scope
    symTable_.pushScope();

    // Insert parameters into function scope
    for (auto& param : node->params) {
        if (param) {
            if (symTable_.lookupCurrentScope(param->name)) {
                error(param->line, param->column,
                      SemanticErrorCode::ERR_REDEFINED_SYMBOL,
                      "redefinition of parameter '" + param->name + "'");
            } else {
                auto pSym = std::make_unique<Symbol>(param->name,
                                                      SymbolKind::PARAM,
                                                      param->line, false);
                symTable_.insert(std::move(pSym));
            }
        }
    }

    // Analyze function body
    auto* savedFunc = currentFunction_;
    currentFunction_ = node;

    if (node->body) {
        node->body->accept(*this);
    }

    // Check return paths for int-returning functions
    if (node->returnType && node->returnType->is_int()) {
        if (node->body && !checkReturnPaths(node->body.get(), true)) {
            error(node->line, node->column,
                  SemanticErrorCode::ERR_MISSING_RETURN,
                  "function '" + node->name +
                  "' is declared to return int but not all paths return a value");
        }
    }

    currentFunction_ = savedFunc;
    symTable_.popScope();
}

void SemanticAnalyzer::visit(Param* node) {
    // Parameters are handled in visit(FuncDef*)
    // Individual Param node visitation is not called separately
}

void SemanticAnalyzer::visit(Block* node) {
    symTable_.pushScope();
    for (auto& stmt : node->statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
    symTable_.popScope();
}

void SemanticAnalyzer::visit(IfStmt* node) {
    if (node->cond) {
        checkExpr(node->cond.get());
    }
    if (node->then) {
        node->then->accept(*this);
    }
    if (node->else_) {
        node->else_->accept(*this);
    }
}

void SemanticAnalyzer::visit(WhileStmt* node) {
    if (node->cond) {
        checkExpr(node->cond.get());
    }
    loopDepth_++;
    if (node->body) {
        node->body->accept(*this);
    }
    loopDepth_--;
}

void SemanticAnalyzer::visit(BreakStmt* node) {
    if (loopDepth_ == 0) {
        error(node->line, node->column,
              SemanticErrorCode::ERR_BREAK_OUTSIDE_LOOP,
              "'break' statement not within a loop");
    }
}

void SemanticAnalyzer::visit(ContinueStmt* node) {
    if (loopDepth_ == 0) {
        error(node->line, node->column,
              SemanticErrorCode::ERR_CONTINUE_OUTSIDE_LOOP,
              "'continue' statement not within a loop");
    }
}

void SemanticAnalyzer::visit(ReturnStmt* node) {
    if (currentFunction_) {
        bool funcReturnsInt = currentFunction_->returnType &&
                              currentFunction_->returnType->is_int();
        bool funcReturnsVoid = currentFunction_->returnType &&
                               currentFunction_->returnType->is_void();

        if (funcReturnsInt && !node->value) {
            error(node->line, node->column,
                  SemanticErrorCode::ERR_RETURN_TYPE_MISMATCH,
                  "function '" + currentFunction_->name +
                  "' expects a return value of type int");
        }
        if (funcReturnsVoid && node->value) {
            error(node->line, node->column,
                  SemanticErrorCode::ERR_RETURN_TYPE_MISMATCH,
                  "void function '" + currentFunction_->name +
                  "' should not return a value");
        }
    }
    if (node->value) {
        checkExpr(node->value.get());
    }
}

void SemanticAnalyzer::visit(ExprStmt* node) {
    if (node->expression) {
        checkExpr(node->expression.get());
    }
}

void SemanticAnalyzer::visit(AssignStmt* node) {
    Symbol* sym = lookupSymbol(node->name);
    if (!sym) {
        error(node->line, node->column,
              SemanticErrorCode::ERR_UNDECLARED_IDENT,
              "use of undeclared identifier '" + node->name + "'");
        return;
    }
    if (sym->isConst) {
        error(node->line, node->column,
              SemanticErrorCode::ERR_CONST_REASSIGN,
              "cannot assign to const variable '" + node->name + "'");
    }
    if (sym->kind == SymbolKind::FUNCTION) {
        error(node->line, node->column,
              SemanticErrorCode::ERR_VOID_FUNC_CALL_EXPR,
              "cannot assign to function '" + node->name + "'");
    }
    if (node->value) {
        checkExpr(node->value.get());
    }
    sym->isUsed = true;
}

void SemanticAnalyzer::visit(EmptyStmt* node) {
    // Nothing to check
}

void SemanticAnalyzer::visit(BinaryExpr* node) {
    if (node->left) {
        checkExpr(node->left.get());
    }
    if (node->right) {
        checkExpr(node->right.get());
    }
    // Check for division by zero in constant expressions
    if ((node->op == BinaryOp::Div ||
         node->op == BinaryOp::Mod) && node->right) {
        auto rightVal = evaluateConstExpr(node->right.get());
        if (rightVal.has_value() && rightVal.value() == 0) {
            error(node->line, node->column,
                  SemanticErrorCode::ERR_DIVIDE_BY_ZERO,
                  "division by zero");
        }
    }
}

void SemanticAnalyzer::visit(UnaryExpr* node) {
    if (node->operand) {
        checkExpr(node->operand.get());
    }
}

void SemanticAnalyzer::visit(Identifier* node) {
    Symbol* sym = lookupSymbol(node->name);
    if (!sym) {
        error(node->line, node->column,
              SemanticErrorCode::ERR_UNDECLARED_IDENT,
              "use of undeclared identifier '" + node->name + "'");
        return;
    }
    sym->isUsed = true;
    // Check use-before-declaration for local symbols
    if (!sym->isGlobal && sym->line > node->line) {
        error(node->line, node->column,
              SemanticErrorCode::ERR_USE_BEFORE_DECL,
             "use of '" + node->name + "' before its declaration");
    }
}

void SemanticAnalyzer::visit(Number* node) {
    // Numeric literals are always valid
}

void SemanticAnalyzer::visit(FuncCall* node) {
    Symbol* sym = lookupSymbol(node->name);
    if (!sym) {
        error(node->line, node->column,
              SemanticErrorCode::ERR_UNDECLARED_IDENT,
              "use of undeclared function '" + node->name + "'");
        return;
    }
    if (sym->kind != SymbolKind::FUNCTION) {
        error(node->line, node->column,
              SemanticErrorCode::ERR_UNDECLARED_IDENT,
              "'" + node->name + "' is not a function");
        return;
    }

    // Check argument count
    int expectedArgs = sym->paramCount;
    int actualArgs = static_cast<int>(node->args.size());
    if (expectedArgs != actualArgs) {
        error(node->line, node->column,
              SemanticErrorCode::ERR_WRONG_ARG_COUNT,
              "function '" + node->name + "' expects " +
              std::to_string(expectedArgs) + " arguments, but " +
              std::to_string(actualArgs) + " were provided");
    }

    // Check all argument expressions
    for (auto& arg : node->args) {
        if (arg) {
            checkExpr(arg.get());
        }
    }

    sym->isUsed = true;
}

// =====================================================================
// Return path analysis
// =====================================================================

bool SemanticAnalyzer::checkReturnPaths(Block* block, bool expectReturn) {
    if (!expectReturn) return true;
    bool hasReturn = false;

    for (auto& stmt : block->statements) {
        if (!stmt) continue;

        // A direct return statement guarantees return on this path
 if (dynamic_cast<ReturnStmt*>(stmt.get())) {
            hasReturn = true;
            continue;
        }

        // For if-statements, both branches must return
        if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt.get())) {
            bool thenHasReturn = false;
            bool elseHasReturn = false;

            if (ifStmt->then) {
                if (auto* thenBlock =
                        dynamic_cast<Block*>(ifStmt->then.get())) {
                    thenHasReturn = checkReturnPaths(thenBlock, true);
                } else if (dynamic_cast<ReturnStmt*>(
                               ifStmt->then.get())) {
                    thenHasReturn = true;
                }
            }

            if (ifStmt->else_) {
                if (auto* elseBlock =
                        dynamic_cast<Block*>(ifStmt->else_.get())) {
                    elseHasReturn = checkReturnPaths(elseBlock, true);
                } else if (dynamic_cast<ReturnStmt*>(
                               ifStmt->else_.get())) {
                    elseHasReturn = true;
                }
            } else {
                // No else branch: there is a path without return
                elseHasReturn = false;
            }

            if (thenHasReturn && elseHasReturn) {
                hasReturn = true;
            }
        }

        // While-loops may not execute, so they do not count as a definite
        // return
    }

    return hasReturn;
}

} // namespace toyc
