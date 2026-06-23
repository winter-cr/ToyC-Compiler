#include "SemanticAnalyzer.h"
#include "errors.h"

namespace semantic {

SemanticAnalyzer::SemanticAnalyzer() : constExprEval_(this) {
}

SemanticAnalyzer::~SemanticAnalyzer() {
    reset();
}

void SemanticAnalyzer::reset() {
    symbolTable_.reset();
    errors_.clear();
}

bool SemanticAnalyzer::analyze(ast::ASTNode* root) {
    reset();
    if (root) {
        auto* compUnit = dynamic_cast<ast::CompUnit*>(root);
        if (!compUnit) {
            addError(0, 0, SemanticErrorCode::ERR_UNDECLARED_IDENT, "invalid AST root");
            return true;
        }
        visitCompUnit(compUnit);
    }
    return hasErrors();
}

void SemanticAnalyzer::addError(int line, int column, SemanticErrorCode code, const std::string& msg) {
    errors_.emplace_back(line, column, code, msg);
}

Symbol* SemanticAnalyzer::lookupGlobal(const std::string& name) {
    return symbolTable_.lookup(name);
}

Symbol* SemanticAnalyzer::lookupLocal(const std::string& name) {
    return symbolTable_.lookup(name);
}

bool SemanticAnalyzer::getConstantValue(const std::string& name, int* out_value) {
    Symbol* sym = lookupGlobal(name);
    if (sym && sym->isConstant() && sym->getConstValue().has_value()) {
        *out_value = sym->getConstValue().value();
        return true;
    }
    return false;
}

void SemanticAnalyzer::visitCompUnit(ast::CompUnit* node) {
    // 收集全局声明
    for (auto* decl : node->getGlobalDecls()) {
        if (auto* varDecl = dynamic_cast<ast::VarDecl*>(decl)) {
            visitVarDecl(varDecl, true);
        } else if (auto* constDecl = dynamic_cast<ast::ConstDecl*>(decl)) {
            visitConstDecl(constDecl, true);
        }
    }

    // 收集函数定义
    for (auto* func : node->getFunctions()) {
        visitFuncDef(func);
    }

    // 检查main函数
    checkMainFunction();
}

void SemanticAnalyzer::visitVarDecl(ast::VarDecl* decl, bool isGlobal) {
    // 检查重定义
    if (symbolTable_.isDefinedInCurrentScope(decl->name)) {
        addError(decl->line, decl->column, SemanticErrorCode::ERR_REDEFINED_SYMBOL,
                "symbol '" + decl->name + "' redefined in the same scope");
        return;
    }

    // 创建符号（目前只支持int类型，忽略decl->type）
    auto sym = std::make_unique<Symbol>(
        isGlobal ? SymbolKind::GLOBAL_VAR : SymbolKind::LOCAL_VAR,
        decl->name,
        BasicType::INT,
        decl->isConst,
        decl->line
    );
    symbolTable_.insert(std::move(sym));
}

void SemanticAnalyzer::visitConstDecl(ast::ConstDecl* decl, bool isGlobal) {
    // 检查重定义
    if (symbolTable_.isDefinedInCurrentScope(decl->name)) {
        addError(decl->line, decl->column, SemanticErrorCode::ERR_REDEFINED_SYMBOL,
                "symbol '" + decl->name + "' redefined in the same scope");
        return;
    }

    // 必须有初始化表达式
    if (!decl->initExpr) {
        addError(decl->line, decl->column, SemanticErrorCode::ERR_CONST_INIT_UNDEFINED,
                "const '" + decl->name + "' must have an initializer");
        return;
    }

    // 常量求值
    auto evalResult = constExprEval_.evaluate(decl->initExpr);
    if (!evalResult.value.has_value()) {
        addError(decl->line, decl->column, evalResult.error,
                "const '" + decl->name + "' initializer is not a constant expression");
        return;
    }
    int value = evalResult.value.value();

    // 创建常量符号
    auto sym = std::make_unique<Symbol>(
        isGlobal ? SymbolKind::GLOBAL_CONST : SymbolKind::LOCAL_CONST,
        decl->name,
        BasicType::INT,
        true,
        decl->line
    );
    sym->constValue = value;
    symbolTable_.insert(std::move(sym));
}

void SemanticAnalyzer::visitFuncDef(ast::FuncDef* func) {
    // Day 2: 只收集函数签名，不进入函数体

    // 检查重定义
    if (symbolTable_.lookup(func->name)) {
        addError(func->line, 0, SemanticErrorCode::ERR_REDEFINED_SYMBOL,
                "function '" + func->name + "' redefined");
        return;
    }

    // 返回类型（暂时只支持int/void）
    BasicType retType = BasicType::INT;  // 简化：默认int
    if (func->returnType) {
        // 如果没有具体类型信息，仍用INT
    }

    // 创建函数符号
    std::vector<std::pair<std::string, BasicType>> params;
    for (auto* param : func->params) {
        params.emplace_back(param->name, BasicType::INT);  // 简化：参数都是int
    }

    auto sym = Symbol::createFunction(func->name, retType, std::move(params), func->line);
    symbolTable_.insert(std::move(sym));
}

void SemanticAnalyzer::checkMainFunction() {
    Symbol* mainSym = symbolTable_.lookup("main");

    if (!mainSym) {
        addError(1, 0, SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE,
                "main function not defined");
        return;
    }

    if (!mainSym->isFunction()) {
        addError(mainSym->line, 0, SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE,
                "main must be a function");
        return;
    }

    // 检查返回类型
    if (mainSym->getReturnType() != BasicType::INT) {
        addError(mainSym->line, 0, SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE,
                "main must return int");
        return;
    }

    // 检查参数：必须为空（表示(void)）
    const auto& params = mainSym->getParams();
    if (!params.empty()) {
        addError(mainSym->line, 0, SemanticErrorCode::ERR_INVALID_MAIN_SIGNATURE,
                "main must have no parameters (use 'void')");
    }
}

} // namespace semantic
