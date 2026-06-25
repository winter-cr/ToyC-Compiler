#ifndef TOYC_SYMBOL_TABLE_H
#define TOYC_SYMBOL_TABLE_H

#include "semantic/Symbol.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace toyc {

class SymbolTable {
public:
    SymbolTable();

    void pushScope();
    void popScope();

    bool insert(std::unique_ptr<Symbol> sym);

    Symbol* lookup(const std::string& name);
    Symbol* lookupCurrentScope(const std::string& name) const;
    Symbol* lookupGlobal(const std::string& name) const;

    int currentLevel() const;

private:
    std::vector<std::unordered_map<std::string, std::unique_ptr<Symbol>>> scopes_;
};

} // namespace toyc

#endif
