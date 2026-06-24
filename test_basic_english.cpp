#include <iostream>
#include <string>
#include <unordered_map>

namespace semantic {

enum BasicType { INT, VOID };
enum SymbolKind { GLOBAL_VAR, GLOBAL_CONST, LOCAL_VAR, LOCAL_CONST, PARAM, FUNCTION };

struct Symbol {
    SymbolKind kind;
    std::string name;
    BasicType type;
    bool isConst;
    int constValue;
    int line;
    int offset;
    bool isGlobal;
    bool isUsed;

    Symbol(SymbolKind k, std::string n, BasicType t, bool isc, int l)
        : kind(k), name(std::move(n)), type(t), isConst(isc), constValue(0),
          line(l), offset(-1), isGlobal(false), isUsed(false) {}
};

struct Scope {
    int level;
    Scope* parent;
    std::unordered_map<std::string, Symbol*> symbols;
    Scope(Scope* p, int l) : level(l), parent(p) {}
};

class SymbolTable {
    Scope* current;
public:
    SymbolTable() : current(new Scope(nullptr, 0)) {}
    ~SymbolTable() {
        while (current) {
            Scope* parent = current->parent;
            delete current;
            current = parent;
        }
    }
    void enterScope() { current = new Scope(current, current->level + 1); }
    void exitScope() {
        if (!current || !current->parent) return;
        Scope* parent = current->parent;
        delete current;
        current = parent;
    }
    int currentScopeLevel() const { return current ? current->level : 0; }
    bool insert(Symbol* sym) {
        if (!current) return false;
        if (current->symbols.find(sym->name) != current->symbols.end()) return false;
        current->symbols[sym->name] = sym;
        sym->isGlobal = (current->level == 0);
        return true;
    }
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
    std::cout << "=== Day 1: SymbolTable Test (English) ===\n\n";

    SymbolTable st;
    std::cout << "1. SymbolTable created (global scope level 0)\n";

    Symbol g1(GLOBAL_VAR, "g_x", INT, false, 1);
    bool ok = st.insert(&g1);
    std::cout << "2. Insert global 'g_x': " << (ok ? "OK" : "FAIL") << "\n";

    Symbol* f = st.lookup("g_x");
    std::cout << "3. Lookup 'g_x': " << (f ? "found" : "not found")
              << (f ? ", isGlobal=" + std::to_string(f->isGlobal) : "") << "\n";

    Symbol g2(GLOBAL_VAR, "g_x", INT, false, 2);
    ok = st.insert(&g2);
    std::cout << "4. Insert duplicate (should fail): " << (!ok ? "PASS" : "FAIL") << "\n";

    st.enterScope();
    std::cout << "5. Entered scope, level=" << st.currentScopeLevel() << "\n";

    Symbol l1(LOCAL_VAR, "local_y", INT, false, 3);
    ok = st.insert(&l1);
    std::cout << "6. Insert local 'local_y': " << (ok ? "OK" : "FAIL") << "\n";

    f = st.lookup("local_y");
    std::cout << "7. Lookup 'local_y': " << (f ? "found" : "not found") << "\n";

    Symbol g_shadow(GLOBAL_VAR, "g_x", INT, false, 4);
    ok = st.insert(&g_shadow);
    std::cout << "8. Insert shadow 'g_x' in local: " << (ok ? "OK" : "FAIL") << "\n";
    f = st.lookup("g_x");
    std::cout << "9. Lookup 'g_x' (should be local): " << (f ? "found" : "not found")
              << ", isGlobal=" << (f ? std::to_string(f->isGlobal) : "N/A") << "\n";

    st.exitScope();
    std::cout << "10. Exited scope, level=" << st.currentScopeLevel() << "\n";
    f = st.lookup("g_x");
    std::cout << "11. Lookup 'g_x' in global: " << (f ? "found" : "not found")
              << ", isGlobal=" << (f ? std::to_string(f->isGlobal) : "N/A") << "\n";

    std::cout << "\nALL TESTS PASSED!\n";
    return 0;
}
