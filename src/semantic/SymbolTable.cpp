#include "SymbolTable.h"
#include <stdexcept>

namespace semantic {

SymbolTable::SymbolTable() : scopeLevel_(0) {
    // 初始化全局作用域（level 0）
    scopeStack_.push_back(std::make_unique<Scope>(0));
}

void SymbolTable::reset() {
    scopeStack_.clear();
    scopeLevel_ = 0;
    // 重新初始化全局作用域
    scopeStack_.push_back(std::make_unique<Scope>(0));
}

void SymbolTable::enterScope() {
    scopeLevel_++;
    scopeStack_.push_back(std::make_unique<Scope>(scopeLevel_));
}

void SymbolTable::exitScope() {
    if (scopeStack_.empty() || scopeStack_.size() == 1) {
        throw std::runtime_error("SymbolTable: exitScope() on empty stack or trying to exit global scope");
    }
    scopeStack_.pop_back();
    scopeLevel_--;
}

bool SymbolTable::insert(std::unique_ptr<Symbol> sym) {
    if (scopeStack_.empty()) {
        return false;
    }
    Scope* current = scopeStack_.back().get();

    // 检查当前作用域是否已存在同名符号
    if (current->symbols.find(sym->name) != current->symbols.end()) {
        return false;
    }

    current->symbols[sym->name] = std::move(sym);
    current->symbols[sym->name]->isGlobal = (current->level == 0);  // 标记是否为全局
    return true;
}

Symbol* SymbolTable::lookup(const std::string& name) {
    // 从栈顶（当前作用域）向下搜索，模拟遮蔽规则
    for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
        auto& symbols = it->get()->symbols;
        auto found = symbols.find(name);
        if (found != symbols.end()) {
            return found->second.get();
        }
    }
    return nullptr;  // 未找到
}

Symbol* SymbolTable::lookupCurrentScope(const std::string& name) {
    if (scopeStack_.empty()) return nullptr;
    Scope* current = scopeStack_.back().get();
    auto found = current->symbols.find(name);
    return found != current->symbols.end() ? found->second.get() : nullptr;
}

bool SymbolTable::isDefinedInCurrentScope(const std::string& name) const {
    if (scopeStack_.empty()) return false;
    const Scope* current = scopeStack_.back().get();
    return current->symbols.find(name) != current->symbols.end();
}

const std::unordered_map<std::string, std::unique_ptr<Symbol>>& SymbolTable::currentScopeSymbols() const {
    if (scopeStack_.empty()) {
        static std::unordered_map<std::string, std::unique_ptr<Symbol>> empty;
        return empty;
    }
    return scopeStack_.back()->symbols;
}

}  // namespace semantic
