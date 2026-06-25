#include "semantic/SymbolTable.h"

namespace toyc {

SymbolTable::SymbolTable() {
    pushScope();
}

void SymbolTable::pushScope() {
    scopes_.emplace_back();
}

void SymbolTable::popScope() {
    if (scopes_.size() <= 1) return;
    scopes_.pop_back();
}

bool SymbolTable::insert(std::unique_ptr<Symbol> sym) {
    if (!sym) return false;
    auto& current = scopes_.back();
    if (current.count(sym->name) > 0) return false;
    current[sym->name] = std::move(sym);
    return true;
}

Symbol* SymbolTable::lookup(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.get();
    }
    return nullptr;
}

Symbol* SymbolTable::lookupCurrentScope(const std::string& name) const {
    auto& current = scopes_.back();
    auto found = current.find(name);
    if (found != current.end()) return found->second.get();
    return nullptr;
}

Symbol* SymbolTable::lookupGlobal(const std::string& name) const {
    if (scopes_.empty()) return nullptr;
    auto& global = scopes_.front();
    auto found = global.find(name);
    if (found != global.end()) return found->second.get();
    return nullptr;
}

int SymbolTable::currentLevel() const {
    return static_cast<int>(scopes_.size()) - 1;
}

} // namespace toyc
