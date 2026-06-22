#include <iostream>
#include "src/semantic/SemanticAnalyzer.h"

// 由于AST还没定义，我们先模拟一个简单的AST结构用于测试
namespace ast {

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(class semantic::SemanticAnalyzer* visitor) = 0;
};

class CompUnit : public ASTNode {
public:
    void accept(semantic::SemanticAnalyzer* visitor) override;
};

void CompUnit::accept(semantic::SemanticAnalyzer* visitor) {
    visitor->visitCompUnit(this);
}

} // namespace ast

int main() {
    std::cout << "=== Semantic Analyzer Test ===\n\n";

    // 测试1: 基础SymbolTable功能
    std::cout << "Test 1: SymbolTable basic operations\n";
    {
        semantic::SymbolTable st;

        // 插入全局变量
        auto sym = std::make_unique<semantic::Symbol>(
            semantic::SymbolKind::GLOBAL_VAR,
            "test_var",
            semantic::BasicType::INT,
            false,
            1
        );
        bool inserted = st.insert(std::move(sym));
        std::cout << "  Insert symbol: " << (inserted ? "OK" : "FAIL") << "\n";

        // 查找
        semantic::Symbol* found = st.lookup("test_var");
        std::cout << "  Lookup: " << (found ? "Found" : "Not found") << "\n";

        // 重复插入应该失败
        auto dup = std::make_unique<semantic::Symbol>(
            semantic::SymbolKind::GLOBAL_VAR,
            "test_var",
            semantic::BasicType::INT,
            false,
            2
        );
        bool dup_inserted = st.insert(std::move(dup));
        std::cout << "  Insert duplicate: " << (dup_inserted ? "OK" : "FAIL (expected)") << "\n";

        // 测试局部作用域
        st.enterScope();
        auto local = std::make_unique<semantic::Symbol>(
            semantic::SymbolKind::LOCAL_VAR,
            "local_var",
            semantic::BasicType::INT,
            false,
            3
        );
        bool local_ok = st.insert(std::move(local));
        std::cout << "  Insert local: " << (local_ok ? "OK" : "FAIL") << "\n";
        st.exitScope();
    }

    std::cout << "\nTest 2: SemanticAnalyzer initialization\n";
    {
        semantic::SemanticAnalyzer analyzer;
        std::cout << "  Analyzer created\n";

        // 测试空AST
        analyzer.analyze(nullptr);
        std::cout << "  analyze(nullptr) returned " << analyzer.hasErrors() << " errors\n";
    }

    std::cout << "\n✅ Basic semantic module compilation and linkage OK!\n";
    std::cout << "Note: Full AST integration pending from module A.\n";

    return 0;
}
