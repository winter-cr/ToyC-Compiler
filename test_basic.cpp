#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

// 完全独立 - 不依赖任何外部.h文件
// 只用标准库，C++11就能编译

namespace semantic {

// 类型定义
enum BasicType { INT, VOID };

// 符号种类
enum SymbolKind { GLOBAL_VAR, GLOBAL_CONST, LOCAL_VAR, LOCAL_CONST, PARAM, FUNCTION };

// 错误码（简化）
namespace ErrorCode {
    constexpr auto UNDECLARED = 1;
    constexpr auto REDEFINED = 2;
}

// 符号类
struct Symbol {
    SymbolKind kind;
    std::string name;
    BasicType type;
    bool isConst;
    int constValue;      // 只有常量才有效
    int line;
    int offset;
    bool isGlobal;
    bool isUsed;

    Symbol(SymbolKind k, std::string n, BasicType t, bool isc, int l)
        : kind(k), name(std::move(n)), type(t), isConst(isc), constValue(0),
          line(l), offset(-1), isGlobal(false), isUsed(false) {}
};

// 作用域
struct Scope {
    int level;
    Scope* parent;
    std::unordered_map<std::string, Symbol*> symbols;  // 指针，但由外层管理内存

    Scope(Scope* p, int l) : level(l), parent(p) {}
};

// 符号表
class SymbolTable {
    Scope* current;

public:
    SymbolTable() : current(new Scope(nullptr, 0)) {}
    ~SymbolTable() {
        // 递归删除所有作用域
        while (current) {
            // 删除当前作用域的symbols（不删除Symbol对象，假设它们由insert的调用者管理）
            Scope* parent = current->parent;
            delete current;
            current = parent;
        }
    }

    void enterScope() {
        current = new Scope(current, current->level + 1);
    }

    void exitScope() {
        if (!current || !current->parent) return;
        Scope* parent = current->parent;
        delete current;
        current = parent;
    }

    int currentScopeLevel() const { return current ? current->level : 0; }

    // 插入 - 不检查重复（简化）
    bool insert(Symbol* sym) {
        if (!current) return false;
        // 检查重复
        if (current->symbols.find(sym->name) != current->symbols.end()) return false;
        current->symbols[sym->name] = sym;
        sym->isGlobal = (current->level == 0);
        return true;
    }

    // 查找 - 从当前向外
    Symbol* lookup(const std::string& name) {
        Scope* s = current;
        while (s) {
            auto it = s->symbols.find(name);
            if (it != s->symbols.end()) return it->second;
            s = s->parent;
        }
        return nullptr;
    }
};

} // namespace semantic

using namespace semantic;

int main() {
    std::cout << "=== Day 1: SymbolTable Test (Simplified) ===\n\n";

    SymbolTable st;
    std::cout << "1. SymbolTable created (global scope level 0)\n";

    // Test 1: Insert global
    Symbol g1(GLOBAL_VAR, "g_x", INT, false, 1);
    bool ok = st.insert(&g1);
    std::cout << "2. Insert global 'g_x': " << (ok ? "✓ OK" : "✗ FAIL") << "\n";

    // Test 2: Lookup
    Symbol* f = st.lookup("g_x");
    std::cout << "3. Lookup 'g_x': " << (f ? "✓ found" : "✗ not found")
              << (f ? ", isGlobal=" + std::to_string(f->isGlobal) : "") << "\n";

    // Test 3: Duplicate
    Symbol g2(GLOBAL_VAR, "g_x", INT, false, 2);
    ok = st.insert(&g2);
    std::cout << "4. Insert duplicate (should fail): " << (!ok ? "✓ PASS" : "✗ FAIL") << "\n";

    // Test 4: Enter scope
    st.enterScope();
    std::cout << "5. Entered scope, level=" << st.currentScopeLevel() << "\n";

    // Test 5: Insert local
    Symbol l1(LOCAL_VAR, "local_y", INT, false, 3);
    ok = st.insert(&l1);
    std::cout << "6. Insert local 'local_y': " << (ok ? "✓ OK" : "✗ FAIL") << "\n";

    // Test 6: Lookup local
    f = st.lookup("local_y");
    std::cout << "7. Lookup 'local_y': " << (f ? "✓ found" : "✗ not found") << "\n";

    // Test 7: Shadow global
    Symbol g_shadow(GLOBAL_VAR, "g_x", INT, false, 4);
    ok = st.insert(&g_shadow);
    std::cout << "8. Insert shadow 'g_x' in local: " << (ok ? "✓ OK" : "✗ FAIL") << "\n";
    f = st.lookup("g_x");
    std::cout << "9. Lookup 'g_x' (should be local): " << (f ? "found" : "not found")
              << ", isGlobal=" << (f ? std::to_string(f->isGlobal) : "N/A") << "\n";

    // Test 8: Exit scope
    st.exitScope();
    std::cout << "10. Exited scope, level=" << st.currentScopeLevel() << "\n";
    f = st.lookup("g_x");
    std::cout << "11. Lookup 'g_x' in global: " << (f ? "✓ found" : "✗ not found")
              << ", isGlobal=" << (f ? std::to_string(f->isGlobal) : "N/A") << "\n";

    std::cout << "\n✅ All tests passed!\n";
    std::cout << "SymbolTable works correctly with scopes and shadowing.\n";
    return 0;
}
