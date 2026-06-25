#ifndef TOYC_SYMBOL_H
#define TOYC_SYMBOL_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace toyc {

enum class SymbolKind {
    GLOBAL_VAR,
    GLOBAL_CONST,
    LOCAL_VAR,
    LOCAL_CONST,
    PARAM,
    FUNCTION
};

struct Symbol {
    std::string name;
    SymbolKind kind;
    bool isConst;
    std::optional<int> constValue;
    int line;
    int offset;
    bool isGlobal;
    bool isUsed;
    int paramCount;
    bool returnsInt;

    Symbol(const std::string& name, SymbolKind kind, int line, bool isGlobal = false)
        : name(name), kind(kind), isConst(false), line(line),
          offset(-1), isGlobal(isGlobal), isUsed(false),
          paramCount(0), returnsInt(false) {}
};

} // namespace toyc

#endif
