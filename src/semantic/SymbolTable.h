#pragma once

#include "Symbol.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <memory>

namespace semantic {

class SymbolTable {
public:
    SymbolTable();
    ~SymbolTable() = default;

    // 作用域管理
    void enterScope();      // 进入新作用域（函数或块）
    void exitScope();       // 退出当前作用域
    int currentScopeLevel() const { return scopeLevel_; }

    // 符号插入（返回成功/失败，失败时表已存在）
    bool insert(std::unique_ptr<Symbol> sym);

    // 符号查找（从当前作用域向外搜索，考虑遮蔽）
    Symbol* lookup(const std::string& name);
    Symbol* lookupCurrentScope(const std::string& name);  // 仅当前作用域

    // 检查当前作用域是否已定义某符号
    bool isDefinedInCurrentScope(const std::string& name) const;

    // 获取当前作用域的所有符号（调试用）
    const std::unordered_map<std::string, std::unique_ptr<Symbol>>& currentScopeSymbols() const;

    // 重置（用于多次分析）
    void clear();
    void reset();

private:
    std::vector<std::unique_ptr<Scope>> scopeStack_;  // 作用域栈，栈顶为当前作用域
    int scopeLevel_;                                  // 当前作用域层级（从0开始）
};

}  // namespace semantic
